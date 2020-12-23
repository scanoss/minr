.PHONY: clean All


minr:
	@echo "----------Building project:[minr]----------"
	"$(MAKE)" -f  "minr.mk"
mz:
	@echo "----------Building project:[mz]----------"
	"$(MAKE)" -f  "mz.mk"

all: minr mz

install:
	cp mz /usr/bin
	cp minr /usr/bin

clean:
	@echo "----------Cleaning project:[ minr]----------"
	"$(MAKE)" -f  "minr.mk" clean
	"$(MAKE)" -f  "mz.mk" clean
	@rm mz
	@rm minr
