#!/bin/bash

passed=0
total=0

for program in bin/tests/*; do
    [ -x $program ] || continue
    category=$(basename "$program")

    for input in tests/"$category"/cases/*.sml; do
        [ -r $input ] || continue
        case=$(basename "$input");
        output=${input%.sml}.out

        res=$("$program" "$input" 2>&1) && res=$(echo "$res"| diff "$output" -)
        if [ $? -eq 0 ]; then
            printf "$category/$case: passed\n"
            ((passed++))
        else
            printf "$category/$case: failed\n\n"
            printf "$res\n\n"
        fi
        ((total++))
    done
done

((total > 0)) && printf "\n"
printf "tests: $passed/$total passed\n"
exit $((passed < total))
