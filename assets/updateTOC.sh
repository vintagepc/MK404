#!/bin/sh
# Update scripting:
echo "# Select a printer to view scripting options:" > Scripting.md
for i in Scripting_*.md; do
	TMP=`basename $i .md`;
	echo "- [${TMP}](${TMP})" >> Scripting.md
done

# Update trace:
echo "# Select a printer to view trace options:" > TraceOptions.md
for i in TraceOptions_*.md; do
	TMP=`basename $i .md`;
	echo "- [${TMP}](${TMP})" >> TraceOptions.md
done
