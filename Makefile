
MAKE = make

.PHONY: all
all:
	$(MAKE) -j -C musl-1.1.18 all
	$(MAKE) -j -C libcxxabi all
	$(MAKE) -j -C libcxx all
	$(MAKE) -j -C asp3_dcre/Debug all
	$(MAKE) -j -C mbed-os all
	$(MAKE) -j -C opencv-lib all
	$(MAKE) -j -C wolfssl-3.15.7 all
	$(MAKE) -j -C zlib-1.2.11 all
	$(MAKE) -j -C zxing all
	$(MAKE) -j -C mbed-gr-libs all
	$(MAKE) -j -C utilities all
	$(MAKE) -j -C curl-7.57.0 all
	$(MAKE) -j -C expat-2.2.5 all
	$(MAKE) -j -C FLIR_Lepton all
	$(MAKE) -j -C HoikuCamApp all
	$(MAKE) -j -C HoikuCam/Debug all

.PHONY: clean
clean:
	$(MAKE) -j -C musl-1.1.18 clean
	$(MAKE) -j -C libcxxabi clean
	$(MAKE) -j -C libcxx clean
	$(MAKE) -j -C asp3_dcre/Debug clean
	$(MAKE) -j -C mbed-os clean
	$(MAKE) -j -C opencv-lib clean
	$(MAKE) -j -C wolfssl-3.15.7 clean
	$(MAKE) -j -C zlib-1.2.11 clean
	$(MAKE) -j -C zxing clean
	$(MAKE) -j -C mbed-gr-libs clean
	$(MAKE) -j -C utilities clean
	$(MAKE) -j -C curl-7.57.0 clean
	$(MAKE) -j -C expat-2.2.5 clean
	$(MAKE) -j -C FLIR_Lepton clean
	$(MAKE) -j -C HoikuCamApp clean
	$(MAKE) -j -C HoikuCam/Debug clean

.PHONY: realclean
realclean:
	$(MAKE) -j -C musl-1.1.18 clean
	$(MAKE) -j -C libcxxabi clean
	$(MAKE) -j -C libcxx clean
	$(MAKE) -j -C asp3_dcre/Debug realclean
	$(MAKE) -j -C mbed-os clean
	$(MAKE) -j -C opencv-lib clean
	$(MAKE) -j -C wolfssl-3.15.7 clean
	$(MAKE) -j -C zlib-1.2.11 clean
	$(MAKE) -j -C zxing clean
	$(MAKE) -j -C mbed-gr-libs clean
	$(MAKE) -j -C utilities clean
	$(MAKE) -j -C curl-7.57.0 clean
	$(MAKE) -j -C expat-2.2.5 clean
	$(MAKE) -j -C FLIR_Lepton clean
	$(MAKE) -j -C HoikuCamApp clean
	$(MAKE) -j -C HoikuCam/Debug realclean
