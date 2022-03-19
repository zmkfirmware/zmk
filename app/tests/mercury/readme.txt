To run tests:

1. Update native_posix.overlay. Default configuration is 2 rows, 2 columns. Mercury needs 4 rows and 12 colums.
DEFAULT:
	kscan: kscan {
		compatible = "zmk,kscan-mock";
		label = "KSCAN_MOCK";

		rows = <2>;
		columns = <2>;
		exit-after;
	};

==> MODIFIED:
	kscan: kscan {
		compatible = "zmk,kscan-mock";
		label = "KSCAN_MOCK";

		rows = <4>;
		columns = <12>;
		exit-after;
	};

2. run the below:
cd /workspaces/zmk/app/tests; clear; west test tests/mercury/sdDn_dUp;