# /etc/profile.d/custom_ps1.sh

export PS1='\[\033[01;38m\][\u@\h:\w]\$ \[\033[00m\]'

alias ls='ls --color=auto'
alias ll='ls -alF --color=auto'
alias la='ls -A --color=auto'
alias l='ls -CF --color=auto'

export TERM=xterm-color

if [ -x /usr/bin/resize ] && termpath="`tty`"; then
    # Make sure we are on a serial console (i.e. the device used
    # starts with /dev/tty),
    # otherwise we confuse e.g. the eclipse launcher which tries
    # do use ssh
    case "$termpath" in
    /dev/console) resize >/dev/null;;
    /dev/tty[A-z]*) resize >/dev/null
    esac
fi

