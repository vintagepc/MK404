#!/bin/sh
# Update scripting:
echo "# Select a printer to view scripting options:" > Scripting.md
for i in Scripting_*.md; do
	echo "- [`basename ${i:10} .md`]($i)" >> Scripting.md
done

# Update trace:
echo "# Select a printer to view trace options:" > TraceOptions.md
for i in TraceOptions_*.md; do
	echo "- [`basename ${i:13} .md`]($i)" >> TraceOptions.md
done
