#! /bin/bash

CHECKS=0
FAILURES=0

while [[ $# -gt 0 ]]; do
    printf '• %s\n' "${1}"

    BASENAME=$(echo "${1}" | sed 's/.s$//g')
    FAILED=0
    TEXT_0x864="${BASENAME}.0x864.text.bin"
    TEXT_NASM="${BASENAME}.nasm.text.bin"
    diff "${TEXT_0x864}" "${TEXT_NASM}" > /dev/null || FAILED=1

    if [[ "${FAILED}" -eq 1 ]]; then
	((FAILURES++))
	printf ' ├─ %s%*s%b\n' ".text section" "33" "" '[ \e[1;31mFAIL\e[0m ]'
    else
	printf ' ├─ %s%*s%b\n' ".text section" "33" "" '[ \e[1;32mOK\e[0m ]'
    fi
    ((CHECKS++))

    FAILED=0
    ELF_0x864="${BASENAME}.0x864.o"
    ELF_NASM="${BASENAME}.nasm.o"
    diff "${ELF_0x864}" "${ELF_NASM}" > /dev/null || FAILED=1

    if [[ "${FAILED}" -eq 1 ]]; then
	((FAILURES++))
	printf ' └─ %s%*s%b\n' "ELF file" "38" "" '[ \e[1;31mFAIL\e[0m ]'
    else
	printf ' └─ %s%*s%b\n' "ELF file" "38" "" '[ \e[1;32mOK\e[0m ]'
    fi
    ((CHECKS++))
    shift
done

if [[ "${FAILURES}" -eq 0 ]]; then
    echo -e "\033[1;32mSUCCESS\033[0m: ${CHECKS} binary checks are identical"
else
    echo -e "\033[1;31mFAILURE\033[0m: ${FAILURES} of ${CHECKS} binary checks differ"

    exit 1
fi
