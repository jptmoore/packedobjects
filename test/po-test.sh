#!/bin/bash
echo "PO test"
###define functions
#usage
usage()
{
    cat << EOF
OPTIONS:
   -h      Show this message
   -f      File name
Example: ./po-test.sh -f menu.xml
EOF
}
# encode (file.xml file.xml file.po loop)
encode() 
{
    xsdEncode=$1
    xmlEncode=$2
    poEncode=$3
    loopEncode=$4
    for i in {1..1}
    do
	./../src/packedobjects --schema $xsdEncode.xsd --in $xmlEncode --out $poEncode.po --loop $loopEncode
    done
}
# decode (file.xml file.xml file.po loop)
decode() 
{
    xsdDecode=$1
    poDecode=$2
    newDecode=$3
    loopDecode=$4
    for i in {1..1}
    do
	./../src/packedobjects --schema $xsdDecode.xsd --in $poDecode.po --out $newDecode.new.xml --loop $loopDecode
    done
}
#differ (file.xml file.new.xml)
differ()
{
    firstFile=$1
    secondFile=$2
    diff --ignore-all-space --ignore-blank-lines -y --suppress-common-lines $firstFile $secondFile.new.xml;
} 
# stats (file.xml file.po file.new.xml)
stats()
{
    xmlStats=$1
    poStats=$2
    newStats=$3
    xmlsize=$(wc -c $xmlStats | egrep "[0-9]{1,}" -o)
    posize=$(wc -c $poStats.po | egrep "[0-9]{1,}" -o)
    ratio=$(echo "scale=3; ${posize}/${xmlsize}" | bc)
    percentage=$(echo "scale=3;(${posize}/${xmlsize})*100" | bc)
    echo -e "File: ${xmlsize} / ${posize} CR ${ratio} - ${percentage}%   \t${file}"
    #format diff as u want
    difference=$(differ $xmlStats $newStats )
    if  [ "$difference" != "" ]
    then
	differ $xmlStats $newStats
	echo ""
    fi
}
clearFile()
{
    
    if [ $# -eq 0 ]
    then
	rm ../examples/*.po
	rm ../examples/*.new.xml
    else
	xmltoClear=${1: 0: -4}
	rm $xmltoClear.new.xml $xmltoClear.po
    fi
}
#init (<file.xml>)
init()
{
if [ $# -eq 0 ]
then
    for file in ../examples/*
    do
	if
	    [ ${file: -4} == ".xml" ]
	then
	    schema=${file: 0:-4}
	    pofile=${file: 0:-4}
	    newxml=${file: 0:-4}
	    encode $schema $file $pofile 1
	    decode $schema $pofile $newxml 1
	    stats $file $pofile $newxml 
	fi
    done  
elif [ $# -eq 1 ]
then
    xmlFile=$1
    if
	[ ${xmlFile: -4} == ".xml" ]
    then
	schema=${xmlFile: 0:-4}
	pofile=${xmlFile: 0:-4}
	newxml=${xmlFile: 0:-4}
	encode $schema $xmlFile $pofile 1
	decode $schema $pofile $newxml 1
	stats $xmlFile $pofile $newxml 
    else
	echo "Enter an XML file"
	usage
    fi
fi
}
### run script
# getopts available not really needed
while getopts ":hf:s:" opt; do
    case $opt in
	h)  usage 
	    exit 1 ;;
	f)  argfile=$OPTARG 
	    ;;
	\?) echo "Invalid option: -$OPTARG" >&2
	    exit 1 ;;
	:)  echo "Option -$OPTARG requires an argument." >&2
	    exit 1 ;;
    esac
done
# main: if   [no arguments] then run all tests
#       elif [2 arguments] then run specific xml
#       else usage
if [ $# -eq 0 ]
then
    init
    clearFile
    exit 1
elif [ $# -eq 2 ]
then
    if [ -f $argfile ]
    then
	init $2
	clearFile $2
	exit 1
    elif [ ! -f $argfile ]
    then 
	echo "file does not exist"
	exit 1
    fi
else
    usage
fi
