# TBD Pipeline 

[[_TOC_]]

## Introduction

### Description

`[DELETE] This section describes what does the pipeline does at a highlevel and the motivation.`

## Definition

`[DELETE] This section describes where the pipeline is and where the code lives.`

| Item   | Description                     |
| ------ | ------------------------------- |
| Name    | AthenaFW|
| Description    | This pipeline does a release build and test on PRs. And a debug/release build and test on scheduled runs.        |
| Link |  https://azurecsi.visualstudio.com/Parthenon/_build?definitionId=301&_a=summary  |
| Definition | YAML: https://azurecsi.visualstudio.com/Parthenon/_git/AthenaFw?path=%2Ftools%2Fpipelines%2Fbuild-pipeline.yml

## Triggers

`[DELETE] This section describes when the pipeline is triggered`

| Item   | Description                     |
| ------ | ------------------------------- |
|PR Trigger on master| Release config only. [Branch policy](https://azurecsi.visualstudio.com/Parthenon/_git/AthenaFw/branchpolicies?refName=refs/heads/master) | 
| Scheduled (Portal)| Both debug/release [2:00am PST](https://azurecsi.visualstudio.com/Parthenon/_apps/hub/ms.vss-ciworkflow.build-ci-hub?_a=edit-build-definition&id=301&view=Tab_Triggers)

## Tokens

`[DELETE] This section describes what tokens/service connections the pipeline needs`

| Item   | Resource                     | Justification                     |
| ------ | ---------------------------- |-----------------------------------|
|[ExpressLogic](https://expresslogic.visualstudio.com/_usersSettings/tokens) | Code (read); Packaging (read)|Access to source code for clonning ThreadX
|[Bemu Service Connection](https://azurecsi.visualstudio.com/Parthenon/_settings/adminservices?resourceId=7a795238-ea9f-45d3-ae48-874b0a3e57ed) | Build (read)|Access to bemu artifacts. |
|[HSP Service Connection](https://azurecsi.visualstudio.com/Parthenon/_settings/adminservices?resourceId=02376dac-80fd-455a-bfa4-72e78728934a) | Build (read)|Access to hsp artifacts. |
|[ONNX Service Connection](https://azurecsi.visualstudio.com/Parthenon/_settings/adminservices?resourceId=e206fc47-827f-4ee4-9d9b-1b715ae8499c) | Build (read)|Access to ONNX artifacts. |
|[PCIe Service Connection](https://azurecsi.visualstudio.com/Parthenon/_settings/adminservices?resourceId=e206fc47-827f-4ee4-9d9b-1b715ae8499c) | Build (read)|Access to PCIe artifacts. |
|[Register Headers Service Connection](https://azurecsi.visualstudio.com/Parthenon/_settings/adminservices?resourceId=e206fc47-827f-4ee4-9d9b-1b715ae8499c) | Build (read)|Access to Athena register header artifacts. |
|[UPT Service Connection](https://azurecsi.visualstudio.com/Parthenon/_settings/adminservices?resourceId=41e1b975-d210-41ea-b36b-7ecec6141c37) | Build (read)|Access to UPT artifacts. |
|Build Token (Regular build token) | Code (read) | Access to git APIs to update package number|

## Resources

`[DELETE] This section describes what resources the pipeline consumes`

| Item   | Resource                     | Justification                     |
| ------ | ---------------------------- |-----------------------------------|
|[1PFW.Fwlibs](https://expresslogic.visualstudio.com/_usersSettings/tokens) | Code repository | Code repo to be built|

## Artifacts

`[DELETE] This section describes what resources the pipeline produces`

| Item   | Resource                     | Justification                     |
| ------ | ---------------------------- |-----------------------------------|
|Compilation artifact | Build artifact | Compiled binaries from the repository |
