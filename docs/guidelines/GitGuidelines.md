# Guidelines for git usage

## Repository Cloning

To clone the repository, use

```pwsh
git clone https://azurecsi.visualstudio.com/Woodinville/_git/Kingsgate.MSCP
```

## Developer Branch Naming

For branches, developers are requested to start off on the latest commit of main branch, unless there's a specific reason not to. On the latest main, create your own developer branch to update your code.
The following convention is recommended since ADO automatically parses a `/` as a delimiter and converts it to a tree: `user/<developer_alias>/<branch_purpose>`. ADO converts this to an easy to read format on the webpage:

--> user  
----> <developer_alias>  
------> <branch_purpose>  

As an example, sherlock holmes would create an example branch as `user/sholmes/demo_branch`.

## Commits

Developers are requested to label commits with sufficient detail to help PR reviewers if they choose to review commit by commit. Feel free to use the message title and the body to add sufficient detail.

## Pull requests

After the feature is implemented or a bug is fixed, submit your code for review using a PR. There are some best practices advised for this submission:

1. **Default merge to main:** For now, all PRs are targeted to the main branch.
2. **Atomic PRs:** Please restrict the changes in one PR to not more than one feature or one bug fix. Do not submit multiple unrelated changes in one PR.
3. **Small and frequent reviews:** If possible, split up a major feature into smaller components and submit each for review and feedback. Splitting up a big chunk of change into smaller changes makers reviewers' work easier and makes the review process more effective. Each small change can be merged individually.
4. **PR Title:** Use a short but self explanatory PR title. Where possible, one recommended format is: <type>:<short_description>. Here, type could be one of the following.
    - feat: new feature.
    - docs: documentation update only, no change in functionality.
    - fix: bug fix.
    - refactor: code refactoring. Code that neither adds nor removes a feature.
    - test: adding or modifying a test.
    - build: additions or changes to building the code (scripts, dependencies, libraries, etc)
    - clean: any code/documentation cleanup. Used instead of refactor/docs when the change is only deletion.
    - chore: any grunt task that has no code impact. Changes in styling, gitignore changes, etc.

    Limit the short description to under 50 characters so that it shows up easily in git log if needed.

5. **PR Description:** The Kingsgate.MSCP repo has a standard PR Description template. Please fill up the template with all relevant details. In a squash merge, the PR description is stored as the commit message body.
6. **ADO Work Item:** It is mandatory to link an ADO work item to the PR for completion.
7. **Reviewers:** Add the relevant stakeholders to the reviewers section. There's a mandatory requirement of 2 reviewers who have not committed code to the branch being reviewed.
8. **PR Completion:** Merge the changes into main once review and approvals are complete.

## Repo Branching Guidelines

For the Kingsgate.MSCP repository, we are following trunk based development, which is widely used, including on the pioneer repo. For more information, please refer [link](<https://trunkbaseddevelopment.com/>). Some salient features are:

- The trunk is the only long lived branch in the repo. All developed code that is accepted to be a part of the product will be merged into this trunk. For us, `main` branch is the trunk.
- All developers branch from the trunk to create or implement features. These implemented features are merged back into the trunk via a PR (pull request).  
![img](./../.img/branching.jpg)  
![img](./../.img/rebase_merge.jpg)  
![img](./../.img/post_merge.jpg)  
- CI/CD and tests always run on any new code merged into the trunk. All tests need to pass before merging can be allowed. No test pass = No merge allowed.
- The latest commit on the trunk is always expected to be release ready. CI/CD and test pass before merge helps us ensure that.
- Any feature is developed jointly with the tests for the feature. No tests = no feature.
- Release from trunk and release branches: Ideally, making a release involves just elevating the candidate commit via a release tag. However, we need to provision for making a patch on a release to fix any bugs found downstream. A short lived release branch is created to support this. The fix/patch is applied on this branch to create an updated release. At the moment, 4 release branches are planned before A0: **M0.1, M0.5, M0.8, M1.0**.  
![img](./../.img/release_branch.png)
- Some cautionary [advice](<https://trunkbaseddevelopment.com/youre-doing-it-wrong/>)
  - If a bug is found in a release, a best practice is to **fix the bug on the trunk first and then copy this to the release branch**, not the other way around.
  - To move code from one branch to another, always cherry pick, never merge. Merging messes up git history over time. And since trunk is always the source of truth, any change should always be implemented on the trunk and then cherry picked to another branch.  
![img](./../.img/patching_release_branch.png)
  
Lastly, some [interesting reads](<https://trunkbaseddevelopment.com/game-changers/>).
