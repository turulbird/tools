STM=$1

if [ -z "$STM" ]; then
	echo "error: use ./make.sh <stm22|stm23|stm24>"
	exit 1
fi

cd $HOME/tools/infobox
"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/tufsbox/devkit/sh4/bin/sh4-linux-gcc -Os -c infobox.c -I"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/cvs/cdk/freetype-2.3.5 -I"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/cvs/cdk/freetype-2.3.5/include -o infobox.o

"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/tufsbox/devkit/sh4/bin/sh4-linux-gcc -Os -c readpng.c -o readpng.o

"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/tufsbox/devkit/sh4/bin/sh4-linux-gcc -Os readpng.o infobox.o -ljpeg -lpng -lfreetype -I"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/cvs/cdk/freetype-2.3.5 -I"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/cvs/cdk/freetype-2.3.5/include -o infobox  
"$HOME"/flashimg/BUILDGIT/checkout_"$STM"/tdt/tufsbox/devkit/sh4/bin/sh4-linux-strip infobox

 