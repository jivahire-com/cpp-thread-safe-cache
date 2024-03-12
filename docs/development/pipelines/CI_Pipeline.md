# Continuous Integration Pipeline

[[_TOC_]]

## Introduction

### Description

Designed to run against any change before that change is merged into a production branch. Validating building, tests, etc...

## Definition

| Item | Description |
| - | - |
| Name | EchoFalls.SoC.FW.CI |
| Link | [Link](https://azurecsi.visualstudio.com/Woodinville/_build?definitionId=3207) |
| Definition | [YAML](../../../tools/pipelines/ci.yaml) |

## Triggers

| Item | Description |
| - | - |
| PR | PRs into origin/main [Branch policy](https://azurecsi.visualstudio.com/Woodinville/_settings/repositories?_a=policiesMid&repo=d81a3cf0-5435-4dd9-8f65-d5ffec3cad6a&refs=refs/heads/main) |

## Tokens

A variable group is used to maintain the needed PATs: [ef-pipeline-config](https://azurecsi.visualstudio.com/Woodinville/_library?itemType=VariableGroups&view=VariableGroupView&variableGroupId=127&path=ef-pipeline-config).

| Item | Resource | Justification |
| - | - | - |
| [1pfw_code_read ](https://azurecsi.visualstudio.com/_usersSettings/tokens) | Code (read); Packaging (read) | Access to source code for cloning shared fpfw libraries. |
| [ms_tsd_code_read](https://dev.azure.com/ms-tsd/_usersSettings/tokens) | Code (read); Packaging (read) | Access to source code for cloning silibs repos. |
| [xware_code_read](https://expresslogic.visualstudio.com/_usersSettings/tokens) | Code (read); Packaging (read) | Access to source code for cloning ThreadX. |

## Resources

| Item | Resource | Justification |
| - | - | - |
| TBD | TBD | TBD |

## Artifacts

| Item | Resource | Justification |
| -| - | - |
| Compilation Artifact | Build Artifact | Compiled binaries from the repository. |
| Testing Artifact | Test Results | Results of all pipeline executed tests. |
