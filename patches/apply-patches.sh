#!/bin/bash

cd ../external
for d in * ; do
    cd $d
    patchfile="../../patches/$d.patch"
    if [[ -f "$patchfile" ]]; then
         # If not a git repo, applying the patch is enough, so we ignore errors here
        git restore --staged . 2>/dev/null
        git restore . 2>/dev/null

        git apply $patchfile
    fi
    cd ..
done

echo "Done."
echo "If you do not see errors above, the patches have been applied successfully."
echo "If the patches could not be applied, the most likely cause is that they already are applied."

