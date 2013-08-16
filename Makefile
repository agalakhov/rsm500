PROGRAM = rsm500

SRCS = common.h \
    syscall.c \
    panic.h panic.c \
    usb-cdc.h usb-cdc.c \
    console.h console.c \
    rsm500-usb.h rsm500-usb.c \
    rsm500.c

ARCH_FLAGS = -mcpu=cortex-m3 -mthumb -msoft-float -DSTM32F1
OPT_FLAGS = -g -O2 -Wall -Wextra -std=c99 -fno-common -flto
LDSCRIPT = stm32-maple.ld
LIBRARY = opencm3_stm32f1

arm-cc = arm-none-eabi-gcc
arm-objcopy = arm-none-eabi-objcopy

################################################################################

.PHONY: all clean
all: $(PROGRAM).bin
clean:
	rm -f $(PROGRAM).elf $(PROGRAM).bin $(objs) $(deps)

####  C rules  ####

flags_c = $(ARCH_FLAGS) $(OPT_FLAGS)
srcs_c = $(filter %.c,$(SRCS))
objs_c = $(srcs_c:.c=.o)
deps_c = $(objs_c:.o=.d)
objs += $(objs_c)
deps += $(deps_c)

$(deps_c): %.d : %.c
	$(arm-cc) $(flags_c) -M -MP -MQ $@ -MQ $(<:%.c=%.o) -o $@ $<
$(objs_c): %.o : %.c
	$(arm-cc) $(flags_c) -c -o $@ $<

####  Link rules  ####

flags_ld = $(ARCH_FLAGS) -T$(LDSCRIPT) -nostartfiles

$(PROGRAM).elf : $(objs)
	$(arm-cc) $(flags_ld) -o $@ $(objs) -l$(LIBRARY)

$(PROGRAM).bin: %.bin : %.elf
	$(arm-objcopy) -O binary $< $@

-include $(deps)
