This is essentially a board definition folder for SimAVR. 

To get stared, clone the modified SimAVR repo at https://github.com/vintagepc/simavr.git and check out the worktree_test branch. the boards_einsy folder may be empty, you will need to cd into the simavr checkout and run `git submodule init` and `git submodule update` from within it to pull down the MK3SIM dependency.

Then, just run `make all` from the root of the SimAVR checkout, it will build all of the boards (Einsy included). 
After the initial build, most of the time you can just run `make` in the boards_einsy folder to build just that. 

Object files are dumped in a folder called `obj-<arch>-<platform>`. Make will automatically link this in the boards_einsy folder.

After that, you should be able to just park your terminal in the boards_einsy folder.
To launch the sim, just run `./Einsy.elf`. It should pick up the firmware and boot, bringing up a display window.
To rebuild any changes, use `make` as above, or for a one-liner that builds and runs, `make Einsy && ./Einsy.elf` to build and run the simulator. (Note this skips the firmware build, you can rebuild that if you made changes in Firmware/ with `make MK3S.afx`


See `Current state of affairs` in the readme for specifics on the state of the included Mk3S.afx firmware and what's included already.
