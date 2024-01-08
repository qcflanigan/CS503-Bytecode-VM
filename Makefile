compiler = gcc
flags = -Wall -std=c11
srcDirectory = sourceCodeFiles
includeDirectory = includeFIles
buildDirectory = buildFiles
binDirectory = bin
executable = bytecodeVM

#get src files
srcFiles = $(wildcard $(srcDirectory)/*.c)

#get obj file names
objFiles = $(patsubst $(srcDirectory)/%.c, $(buildDirectory)/%.o, $(srcFiles))

#get includes
inc = -I$(includeDirectory)

$(buildDirectory)/%.o: $(srcDirectory)/%.c
	$(compiler) $(flags) $(inc) -c $< -o $@

$(executable): $(objFiles)
	$(compiler) $(flags) $(objFiles) -o $(executable)


-PHONY: clean

clean:
	rm -f $(buildDirectory)/*.o $(executable)