
#pragma once

#define USAGE_KEYPAD 0x07
#define USAGE_CONSUMER 0x0C

#define A 0x04  // Keyboard a and A
#define B 0x05  // Keyboard b and B
#define C 0x06  // Keyboard c and C
#define D 0x07  // Keyboard d and D
#define E 0x08  // Keyboard e and E
#define F 0x09  // Keyboard f and F
#define G 0x0A  // Keyboard g and G
#define H 0x0B  // Keyboard h and H
#define I 0x0C  // Keyboard i and I
#define J 0x0D  // Keyboard j and J
#define K 0x0E  // Keyboard k and K
#define L 0x0F  // Keyboard l and L
#define M 0x10  // Keyboard m and M
#define N 0x11  // Keyboard n and N
#define O 0x12  // Keyboard o and O
#define P 0x13  // Keyboard p and P
#define Q 0x14  // Keyboard q and Q
#define R 0x15  // Keyboard r and R
#define S 0x16  // Keyboard s and S
#define T 0x17  // Keyboard t and T
#define U 0x18  // Keyboard u and U
#define V 0x19  // Keyboard v and V
#define W 0x1A  // Keyboard w and W
#define X 0x1B  // Keyboard x and X
#define Y 0x1C  // Keyboard y and Y
#define Z 0x1D  // Keyboard z and Z
#define NUM_1 0x1E  // Keyboard 1 and ! (Exclamation)
#define NUM_2 0x1F  // Keyboard 2 and @ (At sign)
#define NUM_3 0x20  // Keyboard 3 and # (Hash/Pound/Number)
#define NUM_4 0x21  // Keyboard 4 and $ (Dollar/Currency)
#define NUM_5 0x22  // Keyboard 5 and % (Percent)
#define NUM_6 0x23  // Keyboard 6 and ^ (Caret)
#define NUM_7 0x24  // Keyboard 7 and & (Ampersand)
#define NUM_8 0x25  // Keyboard 8 and * (Asterisk)
#define NUM_9 0x26  // Keyboard 9 and ( (Left Parenthesis)
#define NUM_0 0x27  // Keyboard 0 and ) (Right Parenthesis)
#define RET 0x28    // Keyboard Return (Enter)
#define ESC 0x29    // Keyboard Escape
#define BKSP 0x2A   // Keyboard Backspace
#define TAB 0x2B    // Keyboard Tab
#define SPC 0x2C    // Keyboard Space
#define MINUS 0x2D  // Keyboard - and _ (Minus and Underscore) 
#define EQL 0x2E    // Keyboard = and + (Equals and Plus)
#define LBKT 0x2F   // Keyboard [ and { (Left Bracket and Left Curly Bracket)
#define RBKT 0x30   // Keyboard ] and } (Right Bracket and Right Curly Bracket)
#define BSLH 0x31   // Keyboard \ and | (Backslash and Pipe)
#define TILD 0x32   // Keyboard Non-US # and ~ (Pound/Hash/Number and Tilde)
#define SCLN 0x33   // Keyboard ; and : (Semicolon and Colon)
#define QUOT 0x34   // Keyboard ' and " (Appostraphy and Quote)
#define GRAV 0x35   // Keyboard ` and ~ (Grave/Backtick and Tilde)
#define CMMA 0x36   // Keyboard , and < (Comma and Less Than) 
#define DOT  0x37   // Keyboard . and > (Period and Greater Than)
#define FSLH 0x38   // Keyboard / and ? (Forward Slash and Question)
#define CLCK 0x39   // Keyboard Caps Lock
#define F1 0x3A // Keyboard F1
#define F2 0x3B // Keyboard F2
#define F3 0x3C // Keyboard F3
#define F4 0x3D // Keyboard F4
#define F5 0x3E // Keyboard F5
#define F6 0x3F // Keyboard F6
#define F7 0x40 // Keyboard F7
#define F8 0x41 // Keyboard F8
#define F9 0x42 // Keyboard F9
#define F10 0x43    // Keyboard F10
#define F11 0x44    // Keyboard F11
#define F12 0x45    // Keyboard F12

#define PRSC 0x46   // Keyboard Print Screen
#define SCLK 0x47   // Keyboard Scroll Lock
#define PAUS 0x48   // Keyboard Pause/Break
#define INS 0x49    // Keyboard Insert
#define HOME 0x4A   // Keyboard Home
#define PGUP 0x4B   // Keyboard Page Up
#define DEL  0x4C   // Keyboard Delete
#define END  0x4D   // Keyboard End
#define PGDN 0x4E   // Keyboard Page Down
#define RARW 0x4F   // Keyboard Right Arrow
#define LARW 0x50   // Keyboard Left Arrow
#define DARW 0x51   // Keyboard Down Arrow 
#define UARW 0x52   // Keyboard Up Arrow

#define KDIV 0x54   // Keypad / (Divide)
#define KMLT 0x55   // Keypad * (Multiply)
#define KMIN 0x56   // Keypad - (Minus)
#define KPLS 0x57   // Keypad + (Plus)

#define GUI 0x65    // Keyboard GUI (Windows/Command)

#define UNDO 0x7A   // Keyboard Undo
#define CUT 0x7B    // Keyboard Cut
#define COPY 0x7C   // Keyboard Copy
#define PSTE 0x7D   // Keyboard Paste

#define CURU 0xB4   // Keyboard Currency Unit

#define LPRN 0xB6   // Keypad ( (Left Parenthesis)
#define RPRN 0xB7   // Keypad ) (Right Parenthesis)
#define LCUR 0xB8   // Keypad { (Left Curly Bracket)
#define RCUR 0xB9   // Keypad } (Right Curly Bracket)

#define CRRT 0xC3   // Keypad ^ (Caret)
#define PRCT 0xC4   // Keypad % (Percent)
#define LABT 0xC5   // Keypad > (Greater Than)
#define RABT 0xC6   // Keypad < (Less Than)
#define AMPS 0xC7   // Keypad & (Ampersand)
#define PIPE 0xC9   // Keypad | (Pipe)
#define COLN 0xCB   // Keypad : (Colon)
#define HASH 0xCC   // Keypad # (Pound/Hash/Number)
#define KSPC 0xCD   // Keypad Space
#define ATSN 0xCE   // Keypad @ (At Sign)
#define BANG 0xCF   // Keypad ! (Exclamation)

#define PENT 0x58   // Keypad Enter
#define P1  0x59    // Keypad 1 and End
#define P2  0x5A    // Keypad 2 and Down Arrow
#define P3  0x5B    // Keypad 3 and Page Down
#define P4  0x5C    // Keypad 4 and Left Arrow
#define P5  0x5D    // Keypad 5
#define P6  0x5E    // Keypad 6 and Right Arrow
#define P6  0x5F    // Keypad 7 and Home
#define P6  0x60    // Keypad 8 and Up Arrow
#define P7  0x61    // Keypad 9 and Page Up
#define P0  0x62    // Keypad 0 and Insert
#define PDOT 0x63   // Keypad . and Delete

#define LCTL 0xE0   // Keyboard Left Control
#define LSFT 0xE1   // Keyboard Left Shift
#define LALT 0xE2   // Keyboard Left Alt
#define LGUI 0xE3   // Keyboard Left GUI (Windows/Command)
#define RCTL 0xE4   // Keyboard Right Control
#define RSFT 0xE5   // Keyboard Right Shift
#define RALT 0xE6   // Keyboard Right Alt (AltGr)
#define RGUI 0xE7   // Keyboard Right GUI (Windows/Command)

#define VOLU 0x80   // Keyboard Volume Up
#define VOLD 0x81   // Keyboard Volume Down

/* The following are select consumer page usages. Call with "&cp ___" in keymaps. */

#define M_NEXT 0xB5 // Media Next
#define M_PREV 0xB6 // Media Previous
#define M_STOP 0xB7 // Media Stop
#define M_EJCT 0xB8 // Media Eject
#define M_PLAY 0xCD // Media Play/Pause
#define M_MUTE 0xE2 // Media Mute
#define M_VOLU 0xE9 // Media Volume Up
#define M_VOLD 0xEA // Media Volume Down

/* The folowing are modifier masks used for the first byte in the HID report. */

#define MOD_LCTL (1 << 0x00)    // Left Control
#define MOD_LSFT (1 << 0x01)    // Left Shift
#define MOD_LALT (1 << 0x02)    // Left Alt
#define MOD_LGUI (1 << 0x03)    // Left GUI (Windows/Command)
#define MOD_RCTL (1 << 0x04)    // Right Control
#define MOD_RSFT (1 << 0x05)    // Right Shift
#define MOD_RALT (1 << 0x06)    // Right Alt/AltGr
#define MOD_RGUI (1 << 0x07)    // Right GUI (Windows/Command)