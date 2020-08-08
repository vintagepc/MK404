# Contribution Guidelines

This is just a snapshot of some helpful bullet points on contributing. We do welcome contributions of all kinds, but please read through to avoid common pitfalls and save some time. :-)

### Submitting issues

- Before submitting a new issue, please search the existing issues for other reports of the same problem or similar feature requests. If you find one, please consider adding a comment to that issue instead of creating a new one. This helps minimize clutter.

- Please be as detailed as possible when describing and titling your issues. This makes orgainzation and planning much easier for everyone, and reduces delays arising from continued requests for clarification. If you have something you observed that you think may be related to the problem, do mention it as such. It could be key to identifying the source of the problem. Electrons are cheap, so dont' hesitate to add a list of `Possibly related` bullets at the foot of your description. We can always drill into those if necessary.

- Pictures and examples - it can be a hassle, but an exact set of steps that reproduces the issue without fail, screenshots of the problem, and the like can help speed triage and debugging significantly.


### Pull requests

 - If you make an improvement and submit a pull request, please make sure you've read the Contribution Tips below.

 - If we decline your PR or suggest changes, it's nothing personal. This is an open-source project, after all. However, the cleaner and more consistent we can keep the codebase, the easier it is to maintain and debug going forward, and the longer the project (and your contribution) will survive.

 - Ensure your PR has a relevant issue or feature request which can be used for higher level discussion on the topic. Consider a draft PR with a prototype/outline for review, if you're unsure or would like feedback on your implementation.


# Contribution Tips

If you are interested in submitting a fix or improvement, here are a few tidbits you may find useful to know:

- Reference documentation in ref/ (scripting, telemetry, usage readme) are automatically regenerated. The git-bot will push them after it finishes test building your changes for a PR. If you are outside the worktree and do not run workflows, run the appropriate makefile step to regenerate.
- Your changes are expected to compile with -Wall and -Werror on GCC-7.4.0 In addition, pull requests will be run through CPPCheck to look for additional possible pitfalls. These are the default options when building, and CPP check is provided as a convenience target in the makefile.
- Our workflow verifies the pull request compiles successfully across the windows-, ubuntu- and macos- latest platforms. MacOS uses Clang, which is more finicky about some things than GCC-7, so please review the build results and correct any failures.
- As this is a multithreaded application, it is *strongly* suggested your run your code at least once with -fsanitize=thread enabled to look for and fix thread race conditions. This is especially important for the GL Draw() functions as these are called from a separate GL context, less so if your changes do not cross thread boundaries. If, when doing this, you find any not caused in your own changes, please don't hesitate to create an issue so it can be looked at. Run cmake with `-DENABLE_THREAD_SANITY=1` to enable this.
- Where possible, please follow established patterns and styles for maintainability. If you have a reason for not doing so, be sure to explain it with code comments. Remember, code is often written once but needs to be read many times, so the clearer you make it, or the more familiar the pattern already is, the easier it is for others to understand.
- We use `tabs` instead of `spaces`. Pull requests that deliberately change large quantities of whitespace will be rejected, so please check your editor's auto-reformatting options before committing. Pull requests with accidental whitespace changes will have corrections requested.
