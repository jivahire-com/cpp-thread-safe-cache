# Copyright (c) Microsoft Corporation.

import sys
import os
import time
import json
from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parents[1]))
sys.path.append(os.path.join(os.path.dirname(__file__), "kng_pythia_libs"))

from utilities.bmc_utils import run_bmc_commands
from kng_pythia_libs.kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


class cper_functional_test(EchoFallsBaseTest):

    def __init__(
        self,
        name="SOC_CPER_Functional_Test_E2E",
        number="NaN",
        workspace_config=None,
        default_log_home=None,
        fw_payload_path=None,
        dut_platform="KingsgateSVP",
        host_config=None,
        host_name=None,
    ):
        super().__init__(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name,
        )

    def _best_effort_read(self, ch, timeout_seconds=15) -> str:
        end = time.time() + timeout_seconds
        buf = ""
        while time.time() < end:
            try:
                chunk = ch.read_until(key="\n", timeout_seconds=2)
            except Exception:
                chunk = None
            if chunk:
                buf += str(chunk)
                if "mission" in buf.lower() or "whoami" in buf.lower():
                    break
            time.sleep(0.25)
        return buf

    def _safe_int(self, x) -> int:
        try:
            return int(str(x).strip())
        except Exception:
            return -1

    def _looks_like_cper(self, entry: dict) -> bool:
        ddt = (entry.get("DiagnosticDataType") or "").strip().upper()
        msgid = (entry.get("MessageId") or "").strip().upper()
        msg = (entry.get("Message") or "").strip().upper()
        name = (entry.get("Name") or "").strip().upper()

        if ddt == "CPER":
            return True
        if "CPER" in msgid:
            return True
        if "CPER" in name:
            return True
        if "CPER FORMATTED" in msg:
            return True
        return False

    def _curl_json_allow_partial(self, bmc_user: str, bmc_pass: str, url: str, retries: int = 8) -> tuple[dict, str]:
        marker = "__HTTP_CODE__:"
        last_err = ""

        for attempt in range(1, retries + 1):
            cmd = (
                f"curl -sk -u {bmc_user}:{bmc_pass} "
                f"-H 'Accept:application/json' "
                f"-w '\\n{marker}%{{http_code}}\\n' "
                f"'{url}'"
            )
            res = run_bmc_commands(self.dut, self.log, [cmd])

            if not res:
                last_err = "run_bmc_commands returned empty result"
                time.sleep(1.25 * attempt)
                continue

            stdout = (res[0].get("stdout") or "")
            stderr = (res[0].get("stderr") or "")

            if marker not in stdout:
                last_err = f"Missing HTTP marker. stdout_len={len(stdout)} stderr={stderr[:200]}"
                time.sleep(1.25 * attempt)
                continue

            body, http_part = stdout.rsplit(marker, 1)
            http_code = http_part.strip().splitlines()[0].strip() if http_part else ""
            body = (body or "").strip()

            if not body:
                last_err = f"Empty body. HTTP={http_code}. stderr={stderr[:200]}"
                time.sleep(1.25 * attempt)
                continue

            try:
                data = json.loads(body)
            except Exception as e:
                last_err = f"Non-JSON body. HTTP={http_code}. err={e}. body_head={body[:250]}"
                time.sleep(1.25 * attempt)
                continue

            if isinstance(data, dict) and data.get("Members") is not None:
                if http_code and http_code != "200":
                    self.log.warning(f"Redfish returned HTTP={http_code} for {url} but included Members; continuing.")
                if "error" in data:
                    self.log.warning(f"Redfish payload contains 'error' for {url} but included Members; continuing.")
                return data, http_code

            if isinstance(data, dict) and "error" in data:
                last_err = f"Redfish error object. HTTP={http_code}."
                time.sleep(1.25 * attempt)
                continue

            last_err = f"Unexpected JSON structure. HTTP={http_code}."
            time.sleep(1.25 * attempt)

        self.log.error(f"Failed to GET usable JSON from {url} after retries. Last error: {last_err}")
        return {}, ""

    def _list_dump_entries(self, bmc_user: str, bmc_pass: str, bmc_host: str) -> list[dict]:
        url = f"https://{bmc_host}/redfish/v1/Systems/system/LogServices/Dump/Entries"
        data, _http = self._curl_json_allow_partial(bmc_user, bmc_pass, url)
        members = data.get("Members") or []
        return [m for m in members if isinstance(m, dict)]

    def _download_additional_data(self, bmc_user: str, bmc_pass: str, bmc_host: str, additional_uri: str, out_file: str) -> bool:
        url = f"https://{bmc_host}{additional_uri}"
        cmd = (
            f"curl -sk -u {bmc_user}:{bmc_pass} "
            f"-H 'Accept:application/octet-stream' "
            f"-o {out_file} "
            f"'{url}'"
        )
        res = run_bmc_commands(self.dut, self.log, [cmd])
        return bool(res)

    def _validate_cper_dump(self, dump_file: str) -> bool:
        hdr = run_bmc_commands(self.dut, self.log, [f"hexdump -v -C {dump_file} | head -1"])
        first_line = hdr[0].get("stdout", "") if hdr else ""
        if "43 50 45 52" not in first_line:
            self.log.error(f"Invalid CPER header: {first_line}")
            return False
        return True

    def _inject_cper(self, scp_ch) -> None:
        self.log.info("Injecting CPER via SCP UART...")
        scp_ch.write_line(write_string="hm")
        time.sleep(0.5)
        scp_ch.write_line(write_string="hm_submit_sample_cper 5 3")

    def _mission_mode_is_on(self, whoami_output: str) -> bool:
        if not whoami_output:
            return False
        s = whoami_output.lower()

        if "mission" not in s:
            return False

        on_tokens = ["mission mode: on", "mission_mode: on", "mission mode on", "mission_mode on",
                     "enabled", "enable", "true", ":1", "=1"]
        off_tokens = ["mission mode: off", "mission_mode: off", "mission mode off", "mission_mode off",
                      "disabled", "disable", "false", ":0", "=0"]

        if any(t in s for t in off_tokens):
            return False
        if any(t in s for t in on_tokens):
            return True

        return False

    def cper_functional_test(self) -> bool:
        scp_ch = None

        try:
            self.dut.setup()

            cred_path = os.environ.get("SECURE_FILE_PATH")
            if not cred_path:
                self.log.error("SECURE_FILE_PATH is not set")
                return False

            creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            bmc_user = creds["BMC_USER"]
            bmc_pass = creds["BMC_PASSWORD"]

            bmc_host = getattr(self.dut.mb.node_0.dcscm.bmc, "ip", None) or "localhost"
            self.log.info(f"Using BMC host: {bmc_host}")

            scp = self.dut.mb.node_0.soc.primary_die.scp.channel_manager
            scp_ch = scp.get_current_channel()
            scp_ch.open()

            self.log.info("Waiting for SCP Heartbeat...")
            scp_ch.read_until(key="ScpHeartBeat", timeout_seconds=600)
            self.log.info("SCP Heartbeat detected.")
            time.sleep(5)

            # Mission Mode
            try:
                self.log.info("Checking Mission Mode state (best-effort)...")
                scp_ch.write_line(write_string="whoami")
                time.sleep(1)
                out = self._best_effort_read(scp_ch, timeout_seconds=12)
                self.log.info(f"whoami output (raw): {out}")

                if self._mission_mode_is_on(out):
                    self.log.error(
                        "Mission Mode appears to be ON. This test will not change mission mode. "
                        "Run soc_mission_mode_disable.robot before CPER."
                    )
                    return False
            except Exception as mm_e:
                self.log.warning(f"Mission Mode check skipped due to error: {mm_e}")

            run_bmc_commands(self.dut, self.log, ["rm -f cper_dump*.dump cper_dump_latest.dump"])

            before_entries = self._list_dump_entries(bmc_user, bmc_pass, bmc_host)
            before_max_id = max([self._safe_int(e.get("Id")) for e in before_entries] + [-1])
            self.log.info(f"Max Dump Entry Id before injection: {before_max_id}")

            self._inject_cper(scp_ch)

            deadline = time.time() + (10 * 60)
            reinject_after = time.time() + (3 * 60)
            reinjected = False
            chosen = None

            while time.time() < deadline:
                time.sleep(4)

                entries = self._list_dump_entries(bmc_user, bmc_pass, bmc_host)

                new_entries = []
                for e in entries:
                    eid = self._safe_int(e.get("Id"))
                    if eid <= before_max_id:
                        continue
                    if not (e.get("AdditionalDataURI") or "").strip():
                        continue
                    new_entries.append((eid, e))

                if new_entries:
                    cper_candidates = [t for t in new_entries if self._looks_like_cper(t[1])]
                    if cper_candidates:
                        cper_candidates.sort(key=lambda x: x[0], reverse=True)
                        chosen = cper_candidates[0][1]
                        break
                    else:
                        newest_id = max([t[0] for t in new_entries])
                        self.log.info(
                            f"New dump entries detected (newest Id={newest_id}) but none look like CPER yet; waiting..."
                        )

                if (not reinjected) and (time.time() >= reinject_after):
                    self.log.warning("No CPER-like entry detected after 3 minutes; re-injecting CPER once.")
                    self._inject_cper(scp_ch)
                    reinjected = True

            if not chosen:
                self.log.error("No CPER dump found after injection (no new CPER-like entry with AdditionalDataURI).")
                return False

            entry_id = str(chosen.get("Id") or "").strip()
            additional_uri = (chosen.get("AdditionalDataURI") or "").strip()
            created = chosen.get("Created") or ""
            diag = chosen.get("DiagnosticDataType") or ""
            msgid = chosen.get("MessageId") or ""
            self.log.info(
                f"Found CPER-like entry Id={entry_id} Created={created} DiagnosticDataType={diag} MessageId={msgid} "
                f"AdditionalDataURI={additional_uri}"
            )

            dump_file = "cper_dump_latest.dump"
            if not self._download_additional_data(bmc_user, bmc_pass, bmc_host, additional_uri, dump_file):
                self.log.error("Failed to download CPER dump from AdditionalDataURI.")
                return False

            if not self._validate_cper_dump(dump_file):
                return False

            self.log.info("CPER dump validated successfully.")
            return True

        except Exception as e:
            self.log.error(f"Exception during CPER test: {e}")
            return False

        finally:
            try:
                if scp_ch:
                    scp_ch.close()
            except Exception:
                pass
            self.dut.teardown()


__all__ = ["cper_functional_test"]