# Makefile for VisualC/win32 target
# This file was automatically generated from "UnLoader.project": do not edit

#------------------------------------------------------------------------------
#	Compiler definitions
#------------------------------------------------------------------------------

CPP  = cl.exe -nologo -c -D WIN32 -D _WINDOWS
LINK = link.exe -nologo -filealign:512 -incremental:no
AR   = link.exe -lib -nologo

#------------------------------------------------------------------------------
#	symbolic targets
#------------------------------------------------------------------------------

ALL : MAIN
MAIN : UnLoader.exe

#------------------------------------------------------------------------------
#	"UnLoader.exe" target
#------------------------------------------------------------------------------

MAIN = \
	obj/Main.obj \
	obj/UnCore.obj \
	obj/UnObject.obj \
	obj/UnPackage.obj \
	obj/UnRenderer.obj \
	obj/MeshViewer.obj \
	obj/VertMeshViewer.obj \
	obj/VertMeshInstance.obj \
	obj/SkelMeshViewer.obj \
	obj/SkelMeshInstance.obj \
	obj/MaterialViewer.obj \
	obj/GlWindow.obj \
	obj/Math3D.obj \
	obj/ddslib.obj

MAIN_DIRS = \
	obj

UnLoader.exe : $(MAIN_DIRS) $(MAIN)
	echo Creating executable "UnLoader.exe" ...
	$(LINK) -out:"UnLoader.exe" -libpath:"libs" $(MAIN) -subsystem:console

#------------------------------------------------------------------------------
#	compiling source files
#------------------------------------------------------------------------------

OPT_MAIN = -O1 -EHsc -D RENDERING -I libs/include

DEPENDS = \
	Core.h \
	GlWindow.h \
	Math3D.h

obj/GlWindow.obj : GlWindow.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/GlWindow.obj" GlWindow.cpp

DEPENDS = \
	Core.h \
	GlWindow.h \
	Math3D.h \
	MeshInstance.h \
	ObjectViewer.h \
	UnCore.h \
	UnMaterial.h \
	UnMesh.h \
	UnObject.h \
	UnPackage.h

obj/MeshViewer.obj : MeshViewer.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/MeshViewer.obj" MeshViewer.cpp

obj/SkelMeshInstance.obj : SkelMeshInstance.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/SkelMeshInstance.obj" SkelMeshInstance.cpp

obj/SkelMeshViewer.obj : SkelMeshViewer.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/SkelMeshViewer.obj" SkelMeshViewer.cpp

obj/VertMeshInstance.obj : VertMeshInstance.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/VertMeshInstance.obj" VertMeshInstance.cpp

obj/VertMeshViewer.obj : VertMeshViewer.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/VertMeshViewer.obj" VertMeshViewer.cpp

DEPENDS = \
	Core.h \
	GlWindow.h \
	Math3D.h \
	ObjectViewer.h \
	UnAnimNotify.h \
	UnCore.h \
	UnMaterial.h \
	UnMesh.h \
	UnObject.h \
	UnPackage.h

obj/Main.obj : Main.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/Main.obj" Main.cpp

DEPENDS = \
	Core.h \
	GlWindow.h \
	Math3D.h \
	ObjectViewer.h \
	UnCore.h \
	UnMaterial.h \
	UnMesh.h \
	UnObject.h \
	UnPackage.h

obj/MaterialViewer.obj : MaterialViewer.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/MaterialViewer.obj" MaterialViewer.cpp

DEPENDS = \
	Core.h \
	GlWindow.h \
	Math3D.h \
	UnCore.h \
	UnMaterial.h \
	UnObject.h \
	libs/ddslib.h

obj/UnRenderer.obj : UnRenderer.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/UnRenderer.obj" UnRenderer.cpp

DEPENDS = \
	Core.h \
	Math3D.h

obj/Math3D.obj : Math3D.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/Math3D.obj" Math3D.cpp

DEPENDS = \
	Core.h \
	Math3D.h \
	UnCore.h

obj/UnCore.obj : UnCore.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/UnCore.obj" UnCore.cpp

DEPENDS = \
	Core.h \
	Math3D.h \
	UnCore.h \
	UnObject.h \
	UnPackage.h

obj/UnObject.obj : UnObject.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/UnObject.obj" UnObject.cpp

obj/UnPackage.obj : UnPackage.cpp $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/UnPackage.obj" UnPackage.cpp

DEPENDS = \
	libs/ddslib.h

obj/ddslib.obj : libs/ddslib.c $(DEPENDS)
	$(CPP) -MD $(OPT_MAIN) -Fo"obj/ddslib.obj" libs/ddslib.c

#------------------------------------------------------------------------------
#	creating output directories
#------------------------------------------------------------------------------

obj:
	if not exist "obj" mkdir "obj"

