export LS_OPTIONS='-F --color=auto'
eval "`dircolors`"
alias ls='ls $LS_OPTIONS'
if [ -f /workspaces/zmk/zephyr/zephyr-env.sh ]; then
  source /workspaces/zmk/zephyr/zephyr-env.sh
fi
