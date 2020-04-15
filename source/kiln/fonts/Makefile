FONTS = 12x12 32x32 24x32 16x24 16x16 8x16 8x8 6x8 4x6
CFILES = $(foreach f,$(FONTS),font-$(f).c)
PNG2ARR_REPO = ./png2arr
PNG2ARR = $(PNG2ARR_REPO)/build/png2arr


all: $(CFILES)

$(CFILES): font-%.c:%.png $(PNG2ARR)
	@echo $@ $< $*
	$(PNG2ARR) -i $< -o $@ -n __font_$* -d $* -inv

$(PNG2ARR): $(PNG2ARR_REPO)
	$(MAKE) -C $(PNG2ARR_REPO)

$(PNG2ARR_REPO):
	git clone https://github.com/pellepl/png2arr

clean:
	rm $(CFILES)
