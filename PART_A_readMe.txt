DO NOT FORGET SET THE ISO FILE.
then, make clean and make run

In this task,

By default, the strategy function is run. Strategy is the task that PDF mentions.
However, if you want, you can activate another function by comment out "strategy" in "kernel.cpp" file in lines 409-410.

For example,

If you want to see Fork test, comment out 409-410 and activate line 412-413 in "kernel.cpp"
If you want to see Execve test, comment out 409-410 and activate line 415-416 in "kernel.cpp"
If you want to see Waitpid test, comment out 409-410 and activate line 418-419 in "kernel.cpp"


