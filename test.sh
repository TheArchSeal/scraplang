#!/bin/bash

passed=0
total=0

for program in bin/tests/*; do
    [ -x "$program" ] || continue
    category=$(basename "$program")

    for input in tests/"$category"/cases/*.sml; do
        [ -r "$input" ] || continue
        case=$(basename "$input");

        res=$("$program" "$input" 2>&1) && output="${input%.sml}.out" || output="${input%.sml}.err"
        [ -r "$output" ] && res=$(echo "$res" | diff "$output" -)
        if [ $? = 0 ]; then
            printf "$category/$case: passed\n"
            ((passed++))
        else
            printf "$category/$case: failed\n\n"
            printf "%s\n\n" "$res"
        fi
        ((total++))
    done
done

((total > 0)) && printf "\n"
printf "tests: $passed/$total passed\n"
exit $((passed < total))
