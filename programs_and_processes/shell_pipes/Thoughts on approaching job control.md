Thoughts:

* Go back to a working copy of the pipes problem. Shore up the domain model by...
    * Using *process* and *job* abstractions
    * Setting the process groups appropriately
    * Extracting `read`, `eval`, and `exec` methods
        * Including builtin handling!
    * Writing error-handling wrappers (idea from CS:APP)

* Use a toy program that *DOESN'T* ignore any stop signals but *DOES* write to the terminal
    * Stretch goal: what if a child ignores SIGTSTP?
    * As in, let it have default behavior so that it can be stopped, don't override signals!

* Use `exec ./myshell` to set my shell as the only foreground processed
* You'll probably need to use a setjump/long jump to get back to the prompt loop


