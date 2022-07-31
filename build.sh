rm -fr out
mkdir out

cpp_list_tmp=`find . -name "*.cpp"`
inc_dir_list_tmp=`find . ! -path "./.git*" ! -path "./.vscode*" ! -path "./out*"  -type d`
cpp_list=""
inc_dir_list=""

for line in $cpp_list_tmp
do
    cpp_list=$cpp_list" "$line
done

for line in $inc_dir_list_tmp
do
    if [ "" != $line ] && [ "." != $line ]; then
        inc_dir_list=$inc_dir_list" -I "$line
    fi
done

args="$cpp_list $inc_dir_list -o out/bpk -pthread"
echo "g++ $args"

g++ $args


#生成安装包
if (($# >= 1)) && [ $1 == "-b" ]; then
mkdir out/deb
mkdir out/deb/DEBIAN
mkdir out/deb/usr
mkdir out/deb/usr/bin

echo "package: BuildPushKill
version: 1.0
architecture: amd64
maintainer: linzexu <linzexu@xiaomi.com>
description: BuildPushKill" > out/deb/DEBIAN/control

cp out/bpk out/deb/usr/bin

dpkg -b out/deb out/BuildPushKill.deb
fi
