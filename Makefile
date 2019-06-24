# Root Makefile for WslReverse project

all:
	cd common; $(MAKE)
	cd frontend; $(MAKE)
	cd backend; $(MAKE)

clean:
	rm -rf bin

# END