Things I learned:
* Before an exec, both a parent and child will receive a signal from the terminal
* After an exec the calling program signals don't hold, the exec'd program handles signals
* The exec'd program receives the signals as well as the main thread
    * Presumably, that has something to do with the terminal process group id

* A signal handler drops the signal (ie, there's no implicit behavior, the signal handler should implement it)


What this means for the shell:
* Don't expect the signal handler set up in the parent to persist after exec'ing
* Catch SIGTSTP in the parent and forward it to the child
    * Isn't that what I was doing?
* If both the main thread (shell) and the exec'd program receive the signal, how can I control what the exec'd program gets?
* Crux: I need to figure out how to prevent the exec'd program from recieving signals directly.  The shell needs to intercept those (I think?)
    * I don't want the shell to get stopped, so the main thread needs to ignore the signal
    * Since the exec'd program will get the signal, can I just detect what's in the foreground and add it to the jobs array?
    * In bash if I ingore the SIGTSTP signal, the terminal doesn't go bezerk (how does it know?!)