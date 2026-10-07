# vim:set et ts=4 sts=4:
# bash completion for ibus
#
# ibus - The Input Bus
#
# Copyright (c) 2007-2010 Peng Huang <shawn.p.huang@gmail.com>
# Copyright (c) 2007-2010 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <https://www.gnu.org/licenses/>.

if ! type _get_comp_words_by_ref >/dev/null 2>&1; then
if [[ -z ${ZSH_VERSION:+set} ]]; then
_get_comp_words_by_ref ()
{
    local exclude cur_ words_ cword_
    if [ "$1" = "-n" ]; then
        exclude=$2
        shift 2
    fi
    __git_reassemble_comp_words_by_ref "$exclude"
    cur_=${words_[cword_]}
    while [ $# -gt 0 ]; do
        case "$1" in
        cur)
            cur=$cur_
            ;;
        prev)
            prev=${words_[$cword_-1]}
            ;;
        words)
            words=("${words_[@]}")
            ;;
        cword)
            cword=$cword_
            ;;
        esac
        shift
    done
}
else
_get_comp_words_by_ref ()
{
    while [ $# -gt 0 ]; do
        case "$1" in
        cur)
            cur=${COMP_WORDS[COMP_CWORD]}
            ;;
        prev)
            prev=${COMP_WORDS[COMP_CWORD-1]}
            ;;
        words)
            words=("${COMP_WORDS[@]}")
            ;;
        cword)
            cword=$COMP_CWORD
            ;;
        -n)
            # assume COMP_WORDBREAKS is already set sanely
            shift
            ;;
        esac
        shift
    done
}
fi
fi

__ibus_engine()
{
    if [[ "$cmd" == "$prev" ]]; then
        local imes=$( ibus list-engine --name-only 2>/dev/null )
        COMPREPLY=( $( compgen -W "$imes" -- "$cur" | sed "s/^$cur/$cur_/" ))
    fi
}

__ibus_list_engine()
{
   if [[ "$cur" == -* ]]; then
       local options=( --name-only )
       COMPREPLY=( $( compgen -W '${options[@]}' -- "$cur" ))
   fi
}

__ibus()
{
    COMPREPLY=()

    local cur_=$2 prev_=$3 cur words cword prev
    _get_comp_words_by_ref -n =: cur words cword prev

    # echo
    # echo "cur='$cur'"
    # echo "prev='$prev'"
    # echo "words='${words[@]}'"
    # echo "cwords='${cwords[@]}'"

    # Commands
    local cmds=( engine list-engine watch restart exit )

    local i c cmd subcmd
    for (( i=1; i < ${#words[@]}-1; i++)) ; do
        [[ -n $cmd ]] && subcmd=${words[i]} && break
        for c in ${cmds[@]}; do
            [[ ${words[i]} == $c ]] && cmd=$c && break
        done
    done

    case $cmd in
        engine)
            __ibus_engine;
            return 0
            ;;
        list-engine)
            __ibus_list_engine;
            return 0
            ;;
        watch)
            return 0
            ;;
        *)
            COMPREPLY=( $( compgen -W '${cmds[@]}' -- "$cur" ))
            return 0
            ;;
    esac
} &&
complete -o bashdefault -o default -o nospace -F __ibus ibus
