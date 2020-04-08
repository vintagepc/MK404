This is essentially a board definition folder for SimAVR. 

You should be able to drop it in the `examples/` folder of a SimAVR checkout.
Then, just run `make all` from the root of the SimAVR checkout, it will build all of the boards (Einsy included). 
After the initial build, most of the time you can just run 'make` in the boards_einsy folder to build just that. 

Object files are dumped in a folder called `obj-<arch>-<platform>`. Link it to the base folder for easy reference, e.g.:
This is also necessary because it looks for the firwmare files in the same location.
`examples/board_einsy>ln -s obj-x86_64-suse-linux/Einsy.elf .`

After that, you should be able to just park your terminal in the boards_einsy folder.
To launch the sim, just run `./Einsy.elf`. It should pick up the firmware and boot, bringing up a display window.
To rebuild any changes, use `make` as above. 

See `Current state of affairs` in the readme for specifics on the state of the included Mk3S.afx firmware and what's included already.
