# My CL Example Simulation

The cl_my_example example includes a basic test that exercises the Hello World Register as well as the Virtual LED Register. The test writes a value to the Hello World Register and then reads it back. Additionally, it reads the Virtual LED register.

The test can be run from the [verif/scripts] (scripts) directory with one of three different simulators:

```
    $ make TEST=test_my_example
    $ make TEST=test_my_example VCS=1
    $ make TEST=test_my_example QUESTA=1
```

Note that the appropriate simulators must be installed.

