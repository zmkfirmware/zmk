#define DFT 0
#define LWR 1
#define RSE 2
#define ADJ 3

#define Z_BOOT &bootloader
#define Z_BT_0 &bt BT_SEL 0
#define Z_BT_1 &bt BT_SEL 1
#define Z_BT_2 &bt BT_SEL 2
#define Z_BT_3 &bt BT_SEL 3
#define Z_BT_4 &bt BT_SEL 4
#define Z_BT_C &bt BT_CLR
#define Z_VUP  &kp C_VOL_UP
#define Z_VDN  &kp C_VOL_DN
#define Z_MUT  &kp C_MUTE
#define CPW    &caps_word
#define OUT_T  &out OUT_TOG
#define OUT_U  &out OUT_USB
#define OUT_B  &out OUT_BLE
#define EP_T   &ext_power EP_TOG
#define EP_ON  &ext_power EP_ON
#define EP_OF  &ext_power EP_OFF
#define ESCT   &mt TILD ESC
#define FN1    &mt F1 N1
#define FN2    &mt F2 N2
#define FN3    &mt F3 N3 
#define FN4    &mt F4 N4
#define FN5    &mt F5 N5
#define FN6    &mt F6 N6
#define FN7    &mt F7 N7
#define FN8    &mt F8 N8
#define FN9    &mt F9 N9
#define FN10   &mt F10 N0
#define FN11   &mt F11 MINUS
#define ______ &trans
#define XXXXXX &none

// Combo macro
#define COMBO(name, keypress, keypos) \
combo_##name {                        \
    layers = <0 2>;                     \
    timeout-ms = <50>;                \
    bindings = <keypress>;            \
    key-positions = <keypos>;         \
};

// Macro macro
#define MACRO(name, keys)             \
name: name##_macro {                  \
    label = #name;                    \
    compatible = "zmk,behavior-macro";\
    #binding-cells = <0>;             \
    tap-ms = <1>;                     \
    wait-ms = <1>;                    \
    bindings = <keys>;                \
};
