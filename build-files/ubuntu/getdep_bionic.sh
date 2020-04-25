#
#    This file is a part of Tiny-Shading-Language or TSL, an open-source cross
#    platform programming shading language.
#
#    Copyright (c) 2020-2020 by Jiayin Cao - All rights reserved.
#
#    TSL is a free software written for educational purpose. Anyone can distribute
#    or modify it under the the terms of the GNU General Public License Version 3 as
#    published by the Free Software Foundation. However, there is NO warranty that
#    all components are functional in a perfect manner. Without even the implied
#    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#    General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along with
#    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
# 

case "$(uname -s)" in

Linux)
rm -rf dependencies
mkdir dependencies
cd dependencies
sudo apt-get install unzip
sudo wget http://45.63.123.194/tsl_dependencies/linux/llvm_bionic.zip
unzip llvm_bionic.zip
rm llvm_bionic.zip
cd ..
dir
;;

CYGWIN*|MINGW32*|MSYS*)
echo 'MS Windows'
;;

*)
echo 'other OS'
;;
esac
