# Building Zephyr™ Mechanical Keyboard (ZMK) Firmware with Nix

This extension is added by Chris Andreae for MoErgo.

Nix makes setup significantly easier. With this approach West is not needed. You can however still choose to use the ZMK's sanctioned West if you wish.

# To build a target 
In ZMK root directory,

    nix-build -A *target* [-o *output_directory*]
	
An example is 
    nix-build -A glove80_left -o left
	
The output_directory nix creates is a symlink. If you prefer not to rely on symlink (perhaps because you are using WSL on Windows), the following would help

    cp -f $(nix-build -A *target* --no-out-link)/zmk.uf2 .


# To build Glove80
In ZMK root directory,
    cp -f $(nix-build -A glove80_combined --no-out-link)/glove80.uf2 .

# Adding new targets
Edit default.nix and add an target based on zmk

An example is:

    glove80_left = zmk.override {
        board = "glove80_lh";
     };