# General Guidelines

[[_TOC_]]

## Introduction

### Purpose

This document defines the etiquette for participating in this repo.

## General Rules

1. Be consistent. Use the same rules as broadly as possible.

1. Add documentation, this code is intended to be used with future toolchains and platforms we do not have knowledge of. Having good documentation will help people troubleshoot when developing and integrating.

1. Developer documentation and guidelines are live documents, feel encouraged to contribute and review proposals with your peers.

## Repo and dependencies

1. Don't check in binaries into the repository. Any tool required should be added as a package (preferably universal package), either to the general tools or to the specific toolchain tools in (tools\packages*.xml).

1. Open source is welcome, but the license should allow to commercialize and privately distribute our code. Additionally legal and security procedures need to be followed (more on step #6, 7 and 8).

1. Any new dependency, both repos and binaries, for test and production scenarios should be onboarded to [component governance](https://docs.opensource.microsoft.com/using/). Note that no all components can be autodetected and some need to be defined in the [cgmanifest](https://docs.opensource.microsoft.com/tools/cg/features/cgmanifest/) of this repo. They also need to have a fixed version, this to be able to correctly detect issues and preserve previous commit history in a working state.

1. Any security and legal alerts should be triaged early and effectively. These will be generated in the [Governance tab](https://azurecsi.visualstudio.com/Woodinville/_componentGovernance) in Azure Devops. Every time a new dependency is updated or added, an online build should be performed and this tab should be checked for new alerts.

1. Any external dependencies should be upstreamed to the [Microsoft Central OSS repositories](https://www.1eswiki.com/wiki/Central_Feed_Services_(CFS)), or if non existing they should be mirrored either in a new repo prefixed with `mirror.` or in a package in the azure devops feed.

1. Any code-generation scripts shall be written in PowerShell, python, or rust.

1. Any code-generation scripts shall not rely on OS directives, as the repo can be instanced in other machines that do not run Windows.

1. Any new library/service/driver/etc... shall be designed, documented (in docs/<type>) and peer revived using the [template](../.templates/FirmwareDesign.md). All new development shall be associated to a document.

1. Any platform interface demands shall be documented with same template as modules but in doc\Modules\Platform folder.

## DevOps and pipelines

1. Always adhere to the latest recommendations of the 1ES team.

1. Always prefer to host pipelines through `Microsoft Hosted Pipelines` or `1ES Pipelines`. Only use `Self-Hosted pipelines` when required.

1. When creating PAT tokens, create them with the least scope in both terms of Azure Devops Organization and capabilities. The maximum period of time of allowed is 6 months.

1. Always associate PAT tokens with [Service connections](https://azurecsi.visualstudio.com/DuvallFw/_settings/adminservices). Service connections provide a centralize way of understanding what tokens are in use, and most importantly they leave and audit trail for security purposes.

1. When creating pipelines, do not checkout the repo automatically with Azure Devops. Always define the [repo](https://docs.microsoft.com/en-us/azure/devops/pipelines/repos/multi-repo-checkout?view=azure-devops) resource upfront, in case other dependencies are added in the future and this also allows to complement them with a service connection when needed.

1. When downloading packages in a pipeline, if there are packages that live in a different organization prefer to use [upstream connections](https://docs.microsoft.com/en-us/azure/devops/artifacts/concepts/upstream-sources?view=azure-devops). If that is not possible use tasks that work through service connections. Avoid using `PAT` tokens as a variable (even if secret) in the pipeline.

1. When creating a new pipeline, a new entry in the catalog folder (docs/Development/Pipelines) should be created from this [template](../.templates/Pipeline-Definition.md).

1. All triggers and conditions on the pipeline should be defined in the yaml itself, and not through the DevOps interface.

### Feeds and packages

1. Universal packages is our preferred way of packaging. As it provides consisting tooling in both windows and linux.

1. Feeds should not upstream to any public sources.
