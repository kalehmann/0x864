#! /bin/bash

CHECKS=0
FAILURES=0

while [[ $# -gt 0 ]]; do
    FAILED=0
    BASENAME=$(echo "${1}" | sed 's/.s$//g')
    TEXT_0x864="${BASENAME}.0x864.text.bin"
    TEXT_NASM="${BASENAME}.nasm.text.bin"
    diff "${TEXT_0x864}" "${TEXT_NASM}" > /dev/null || FAILED=1

    if [[ "${FAILED}" -eq 1 ]]; then
	((FAILURES++))
	printf '• %s%*s%b\n' "${1}" "$((46 - ${#1}))" "" '[ \e[1;31mFAIL\e[0m ]'
    else
	printf '• %s%*s%b\n' "${1}" "$((46 - ${#1}))" "" '[ \e[1;32mOK\e[0m ]'
    fi

    ((CHECKS++))
    shift
done

if [[ "${FAILURES}" -eq 0 ]]; then
    echo -e "\033[1;32mSUCCESS\033[0m: ${CHECKS} binary files are identical"
else
    echo -e "\033[1;31mFAILURE\033[0m: ${FAILURES} of ${CHECKS} binary files differ"

    exit 1
fi
