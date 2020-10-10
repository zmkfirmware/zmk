export LS_OPTIONS='-F --color=auto'
eval "`dircolors`"
alias ls='ls $LS_OPTIONS'
if [ -f "$WORKSPACE_DIR/zephyr/zephyr-env.sh" ]; then
  source "$WORKSPACE_DIR/zephyr/zephyr-env.sh"
fi
