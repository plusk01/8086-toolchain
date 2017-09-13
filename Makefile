BIN_DIR=bin/

all:
	make --directory assembler/
	make --directory compiler/
	make --directory emulator/

clean:
	make --directory assembler/ clean
	make --directory compiler/ clean
	make --directory emulator/ clean
	rm -rf $(BIN_DIR)
