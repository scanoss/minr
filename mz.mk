##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=mz
ConfigurationName      :=Debug
WorkspacePath          :=/home/osboxes/scanoss
ProjectPath            :=/home/osboxes/scanoss/minr
IntermediateDirectory  :=./build
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=osboxes.org
Date                   :=23/12/20
CodeLitePath           :=/home/osboxes/.codelite
LinkerName             :=/usr/bin/gcc
SharedObjectLinkerName :=/usr/bin/gcc -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="mz.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            := $(IncludeSwitch)/usr/include/  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)./inc $(IncludeSwitch)./external/inc 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)ldb $(LibrarySwitch)pthread $(LibrarySwitch)crypto $(LibrarySwitch)z 
ArLibs                 :=  "ldb" "pthread" "crypto" "z" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O0 -Wall $(Preprocessors)
CFLAGS   :=  -g -O -Wall  $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/src_hex.c$(ObjectSuffix) $(IntermediateDirectory)/src_file.c$(ObjectSuffix) $(IntermediateDirectory)/src_license.c$(ObjectSuffix) $(IntermediateDirectory)/src_help.c$(ObjectSuffix) $(IntermediateDirectory)/src_minr.c$(ObjectSuffix) $(IntermediateDirectory)/external_src_bsort.c$(ObjectSuffix) $(IntermediateDirectory)/external_src_winnowing.c$(ObjectSuffix) $(IntermediateDirectory)/src_string.c$(ObjectSuffix) $(IntermediateDirectory)/src_import.c$(ObjectSuffix) $(IntermediateDirectory)/src_blacklist.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_mz_main.c$(ObjectSuffix) $(IntermediateDirectory)/src_quality.c$(ObjectSuffix) $(IntermediateDirectory)/src_mz.c$(ObjectSuffix) $(IntermediateDirectory)/external_src_crc32c.c$(ObjectSuffix) $(IntermediateDirectory)/src_mz_deflate.c$(ObjectSuffix) $(IntermediateDirectory)/src_copyright.c$(ObjectSuffix) $(IntermediateDirectory)/src_md5.c$(ObjectSuffix) $(IntermediateDirectory)/src_mz_optimise.c$(ObjectSuffix) $(IntermediateDirectory)/src_mz_mine.c$(ObjectSuffix) $(IntermediateDirectory)/src_join.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_license_ids.c$(ObjectSuffix) $(IntermediateDirectory)/src_wfp.c$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./build || $(MakeDirCommand) ./build


$(IntermediateDirectory)/.d:
	@test -d ./build || $(MakeDirCommand) ./build

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/src_hex.c$(ObjectSuffix): src/hex.c $(IntermediateDirectory)/src_hex.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/hex.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_hex.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_hex.c$(DependSuffix): src/hex.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_hex.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_hex.c$(DependSuffix) -MM src/hex.c

$(IntermediateDirectory)/src_hex.c$(PreprocessSuffix): src/hex.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_hex.c$(PreprocessSuffix) src/hex.c

$(IntermediateDirectory)/src_file.c$(ObjectSuffix): src/file.c $(IntermediateDirectory)/src_file.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/file.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_file.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_file.c$(DependSuffix): src/file.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_file.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_file.c$(DependSuffix) -MM src/file.c

$(IntermediateDirectory)/src_file.c$(PreprocessSuffix): src/file.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_file.c$(PreprocessSuffix) src/file.c

$(IntermediateDirectory)/src_license.c$(ObjectSuffix): src/license.c $(IntermediateDirectory)/src_license.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/license.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_license.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_license.c$(DependSuffix): src/license.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_license.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_license.c$(DependSuffix) -MM src/license.c

$(IntermediateDirectory)/src_license.c$(PreprocessSuffix): src/license.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_license.c$(PreprocessSuffix) src/license.c

$(IntermediateDirectory)/src_help.c$(ObjectSuffix): src/help.c $(IntermediateDirectory)/src_help.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/help.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_help.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_help.c$(DependSuffix): src/help.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_help.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_help.c$(DependSuffix) -MM src/help.c

$(IntermediateDirectory)/src_help.c$(PreprocessSuffix): src/help.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_help.c$(PreprocessSuffix) src/help.c

$(IntermediateDirectory)/src_minr.c$(ObjectSuffix): src/minr.c $(IntermediateDirectory)/src_minr.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/minr.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_minr.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_minr.c$(DependSuffix): src/minr.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_minr.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_minr.c$(DependSuffix) -MM src/minr.c

$(IntermediateDirectory)/src_minr.c$(PreprocessSuffix): src/minr.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_minr.c$(PreprocessSuffix) src/minr.c

$(IntermediateDirectory)/external_src_bsort.c$(ObjectSuffix): external/src/bsort.c $(IntermediateDirectory)/external_src_bsort.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/external/src/bsort.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/external_src_bsort.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/external_src_bsort.c$(DependSuffix): external/src/bsort.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/external_src_bsort.c$(ObjectSuffix) -MF$(IntermediateDirectory)/external_src_bsort.c$(DependSuffix) -MM external/src/bsort.c

$(IntermediateDirectory)/external_src_bsort.c$(PreprocessSuffix): external/src/bsort.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/external_src_bsort.c$(PreprocessSuffix) external/src/bsort.c

$(IntermediateDirectory)/external_src_winnowing.c$(ObjectSuffix): external/src/winnowing.c $(IntermediateDirectory)/external_src_winnowing.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/external/src/winnowing.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/external_src_winnowing.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/external_src_winnowing.c$(DependSuffix): external/src/winnowing.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/external_src_winnowing.c$(ObjectSuffix) -MF$(IntermediateDirectory)/external_src_winnowing.c$(DependSuffix) -MM external/src/winnowing.c

$(IntermediateDirectory)/external_src_winnowing.c$(PreprocessSuffix): external/src/winnowing.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/external_src_winnowing.c$(PreprocessSuffix) external/src/winnowing.c

$(IntermediateDirectory)/src_string.c$(ObjectSuffix): src/string.c $(IntermediateDirectory)/src_string.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/string.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_string.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_string.c$(DependSuffix): src/string.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_string.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_string.c$(DependSuffix) -MM src/string.c

$(IntermediateDirectory)/src_string.c$(PreprocessSuffix): src/string.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_string.c$(PreprocessSuffix) src/string.c

$(IntermediateDirectory)/src_import.c$(ObjectSuffix): src/import.c $(IntermediateDirectory)/src_import.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/import.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_import.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_import.c$(DependSuffix): src/import.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_import.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_import.c$(DependSuffix) -MM src/import.c

$(IntermediateDirectory)/src_import.c$(PreprocessSuffix): src/import.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_import.c$(PreprocessSuffix) src/import.c

$(IntermediateDirectory)/src_blacklist.c$(ObjectSuffix): src/blacklist.c $(IntermediateDirectory)/src_blacklist.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/blacklist.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_blacklist.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_blacklist.c$(DependSuffix): src/blacklist.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_blacklist.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_blacklist.c$(DependSuffix) -MM src/blacklist.c

$(IntermediateDirectory)/src_blacklist.c$(PreprocessSuffix): src/blacklist.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_blacklist.c$(PreprocessSuffix) src/blacklist.c

$(IntermediateDirectory)/src_mz_main.c$(ObjectSuffix): src/mz_main.c $(IntermediateDirectory)/src_mz_main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/mz_main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mz_main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mz_main.c$(DependSuffix): src/mz_main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mz_main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mz_main.c$(DependSuffix) -MM src/mz_main.c

$(IntermediateDirectory)/src_mz_main.c$(PreprocessSuffix): src/mz_main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mz_main.c$(PreprocessSuffix) src/mz_main.c

$(IntermediateDirectory)/src_quality.c$(ObjectSuffix): src/quality.c $(IntermediateDirectory)/src_quality.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/quality.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_quality.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_quality.c$(DependSuffix): src/quality.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_quality.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_quality.c$(DependSuffix) -MM src/quality.c

$(IntermediateDirectory)/src_quality.c$(PreprocessSuffix): src/quality.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_quality.c$(PreprocessSuffix) src/quality.c

$(IntermediateDirectory)/src_mz.c$(ObjectSuffix): src/mz.c $(IntermediateDirectory)/src_mz.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/mz.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mz.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mz.c$(DependSuffix): src/mz.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mz.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mz.c$(DependSuffix) -MM src/mz.c

$(IntermediateDirectory)/src_mz.c$(PreprocessSuffix): src/mz.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mz.c$(PreprocessSuffix) src/mz.c

$(IntermediateDirectory)/external_src_crc32c.c$(ObjectSuffix): external/src/crc32c.c $(IntermediateDirectory)/external_src_crc32c.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/external/src/crc32c.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/external_src_crc32c.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/external_src_crc32c.c$(DependSuffix): external/src/crc32c.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/external_src_crc32c.c$(ObjectSuffix) -MF$(IntermediateDirectory)/external_src_crc32c.c$(DependSuffix) -MM external/src/crc32c.c

$(IntermediateDirectory)/external_src_crc32c.c$(PreprocessSuffix): external/src/crc32c.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/external_src_crc32c.c$(PreprocessSuffix) external/src/crc32c.c

$(IntermediateDirectory)/src_mz_deflate.c$(ObjectSuffix): src/mz_deflate.c $(IntermediateDirectory)/src_mz_deflate.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/mz_deflate.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mz_deflate.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mz_deflate.c$(DependSuffix): src/mz_deflate.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mz_deflate.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mz_deflate.c$(DependSuffix) -MM src/mz_deflate.c

$(IntermediateDirectory)/src_mz_deflate.c$(PreprocessSuffix): src/mz_deflate.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mz_deflate.c$(PreprocessSuffix) src/mz_deflate.c

$(IntermediateDirectory)/src_copyright.c$(ObjectSuffix): src/copyright.c $(IntermediateDirectory)/src_copyright.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/copyright.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_copyright.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_copyright.c$(DependSuffix): src/copyright.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_copyright.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_copyright.c$(DependSuffix) -MM src/copyright.c

$(IntermediateDirectory)/src_copyright.c$(PreprocessSuffix): src/copyright.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_copyright.c$(PreprocessSuffix) src/copyright.c

$(IntermediateDirectory)/src_md5.c$(ObjectSuffix): src/md5.c $(IntermediateDirectory)/src_md5.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/md5.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_md5.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_md5.c$(DependSuffix): src/md5.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_md5.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_md5.c$(DependSuffix) -MM src/md5.c

$(IntermediateDirectory)/src_md5.c$(PreprocessSuffix): src/md5.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_md5.c$(PreprocessSuffix) src/md5.c

$(IntermediateDirectory)/src_mz_optimise.c$(ObjectSuffix): src/mz_optimise.c $(IntermediateDirectory)/src_mz_optimise.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/mz_optimise.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mz_optimise.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mz_optimise.c$(DependSuffix): src/mz_optimise.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mz_optimise.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mz_optimise.c$(DependSuffix) -MM src/mz_optimise.c

$(IntermediateDirectory)/src_mz_optimise.c$(PreprocessSuffix): src/mz_optimise.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mz_optimise.c$(PreprocessSuffix) src/mz_optimise.c

$(IntermediateDirectory)/src_mz_mine.c$(ObjectSuffix): src/mz_mine.c $(IntermediateDirectory)/src_mz_mine.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/mz_mine.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mz_mine.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mz_mine.c$(DependSuffix): src/mz_mine.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mz_mine.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mz_mine.c$(DependSuffix) -MM src/mz_mine.c

$(IntermediateDirectory)/src_mz_mine.c$(PreprocessSuffix): src/mz_mine.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mz_mine.c$(PreprocessSuffix) src/mz_mine.c

$(IntermediateDirectory)/src_join.c$(ObjectSuffix): src/join.c $(IntermediateDirectory)/src_join.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/join.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_join.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_join.c$(DependSuffix): src/join.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_join.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_join.c$(DependSuffix) -MM src/join.c

$(IntermediateDirectory)/src_join.c$(PreprocessSuffix): src/join.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_join.c$(PreprocessSuffix) src/join.c

$(IntermediateDirectory)/src_license_ids.c$(ObjectSuffix): src/license_ids.c $(IntermediateDirectory)/src_license_ids.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/license_ids.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_license_ids.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_license_ids.c$(DependSuffix): src/license_ids.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_license_ids.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_license_ids.c$(DependSuffix) -MM src/license_ids.c

$(IntermediateDirectory)/src_license_ids.c$(PreprocessSuffix): src/license_ids.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_license_ids.c$(PreprocessSuffix) src/license_ids.c

$(IntermediateDirectory)/src_wfp.c$(ObjectSuffix): src/wfp.c $(IntermediateDirectory)/src_wfp.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/osboxes/scanoss/minr/src/wfp.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_wfp.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_wfp.c$(DependSuffix): src/wfp.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_wfp.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_wfp.c$(DependSuffix) -MM src/wfp.c

$(IntermediateDirectory)/src_wfp.c$(PreprocessSuffix): src/wfp.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_wfp.c$(PreprocessSuffix) src/wfp.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./build/


