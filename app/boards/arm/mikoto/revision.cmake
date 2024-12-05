board_check_revision(FORMAT MAJOR.MINOR.PATCH
                     DEFAULT_REVISION 5.20.0
                     VALID_REVISIONS
                     5.20.0         # first public release
                     6.1.0  6.3.0   # incompatible pinout change from v5+
                     7.2.0          # addition of MAX17048; compatible pinout with v6+
)
