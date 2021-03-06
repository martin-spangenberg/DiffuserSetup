#!/bin/bash

init=1
tooldaq=1
boostflag=1
zmq=1
msgpack=1
final=1
rootflag=1

while [ ! $# -eq 0 ]
do
    case "$1" in
	--help | -h)
	    helpmenu
	    exit
	    ;;

	--with_root | -r)
	    echo "Installing ToolDAQ with root"
	    rootflag=1 
	    ;;
	
	--no_boost | -b)
            echo "Installing ToolDAQ without boost"
            boostflag=0
	    ;;
	
	--no_init )
	     echo "Installing ToolDAQ without creating ToolDAQ Folder"
	    init=0;
	    ;;

	--no_zmq )
            echo "Installing ToolDAQ without zmq"
            zmq=0
            ;;

	--no_tooldaq )
	    echo "Installing dependancies without ToolDAQ"
	    tooldaq=0
	    ;;

	--no_final )
            echo "Installing ToolDAQ without compiling ToolAnalysis"
            final=0
            ;;

	--ToolDAQ_ZMQ )
            echo "Installing ToolDAQ & ZMQ"
	    boostflag=0
	    rootflag=0
	    final=0
            ;;

	--Boost )
            echo "Installing Boost"
	    init=0
	    tooldaq=0
	    zmq=0
	    final=0
	    rootflag=0
            ;;

	--Root )
            echo "Installing Root"
	    init=0
	    tooldaq=0
	    boostflag=0
	    zmq=0
	    final=0
            ;;
	
	
	--Final )
            echo "Compiling ToolDAQ"
	    init=0
	    tooldaq=0
	    boostflag=0
	    rootflag=0
	    zmq=0
            ;;

    esac
    shift
done

if [ $init -eq 1 ]
then
    
    mkdir ToolDAQ
fi

cd ToolDAQ

if [ $tooldaq -eq 1 ]
then

git clone https://github.com/ToolDAQ/ToolDAQFramework.git
fi

if [ $zmq -eq 1 ]
then
    git clone https://github.com/ToolDAQ/zeromq-4.0.7.git
    
    cd zeromq-4.0.7
    
    ./configure --prefix=`pwd`
    make -j8
    make install
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
    
    cd ../
fi

if [ $boostflag -eq 1 ]
then
    
    git clone https://github.com/ToolDAQ/boost_1_66_0.git
    #wget http://downloads.sourceforge.net/project/boost/boost/1.66.0/boost_1_66_0.tar.gz
    
    #tar zxf boost_1_66_0.tar.gz
    #rm -rf boost_1_66_0.tar.gz
     
    cd boost_1_66_0
    
    mkdir install
    
    ./bootstrap.sh --prefix=`pwd`/install/  > /dev/null 2>/dev/null
    ./b2 install iostreams
    
    export LD_LIBRARY_PATH=`pwd`/install/lib:$LD_LIBRARY_PATH
    cd ../
fi


if [ $rootflag -eq 1 ]
then
    
    wget https://root.cern/download/root_v6.24.00.Linux-ubuntu20-x86_64-gcc9.3.tar.gz
    tar -xvzf root_v6.24.00.Linux-ubuntu20-x86_64-gcc9.3.tar.gz
    rm -rf root_v6.24.00.Linux-ubuntu20-x86_64-gcc9.3.tar.gz
    cd root

    source ./bin/thisroot.sh
    
    cd ../
    
fi

if [ $msgpack -eq 1 ]
then

    git clone https://github.com/msgpack/msgpack-c.git msgpack-3.3.0
    cd msgpack-3.3.0
    git checkout cpp-3.3.0

    cd ../
fi

cd ../

if [ $final -eq 1 ]
then
    
    echo "current directory"
    echo `pwd`
    make clean
    make 
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi
