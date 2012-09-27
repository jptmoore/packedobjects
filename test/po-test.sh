#!/bin/bash
echo "PO test"
init()
{ 
for file in ../examples/*
do
    if [ ${file: -4} == ".xml" ]
    then
	schema=${file: 0:-4}
	pofile=${file: 0:-4}
	newxml=${file: 0:-4}
	encode() 
	{
	    for i in {1..1}
	    do
		./../src/packedobjects --schema $schema.xsd --in $file  --out $pofile.po
	    done
	}
	decode()
	{
	    for i in {1..1}
	    do
		./../src/packedobjects --schema $schema.xsd --in $pofile.po  --out $newxml.new.xml
	    done
	}
	differ()
	{
	    diff --ignore-all-space --ignore-blank-lines -y --suppress-common-lines $file $newxml.new.xml; 
	}

	    encode
	    decode
            # difference=$(differ)
	    xmlsize=$(wc -c $file | egrep "[0-9]{1,}" -o)
	    posize=$(wc -c $pofile.po | egrep "[0-9]{1,}" -o)
	    ratio=$(echo "scale=3; ${posize}/${xmlsize}" | bc)
	    percentage=$(echo "scale=3;(${posize}/${xmlsize})*100" | bc)
	    echo -e "File: ${xmlsize} / ${posize} CR ${ratio} - ${percentage}%   \t${file}"
	    # echo "$difference"
	    differ
    fi
done 
} 
init
rm ../examples/*.po
rm ../examples/*.new.xml
