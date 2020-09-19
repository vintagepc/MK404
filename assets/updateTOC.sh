#!/bin/sh
# Update scripting:
echo "# Select a printer to view scripting options:" > Scripting.md
for i in Scripting-*.md; do
	TMP=`basename $i .md`;
	echo "- [${TMP}](${TMP})" >> Scripting.md
done

# Update trace:
echo "# Select a printer to view trace options:" > Trace-Options.md
for i in Trace-Options-*.md; do
	TMP=`basename $i .md`;
	echo "- [${TMP}](${TMP})" >> Trace-Options.md
done

# Update trace:
echo "# Select a printer to view keyboard controls:" > Key-Controls.md
for i in Key-Controls-*.md; do
	TMP=`basename $i .md`;
	echo "- [${TMP}](${TMP})" >> Key-Controls.md
done
