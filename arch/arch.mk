
# find boot file
ifndef NO_CRT0
  _crt0_proc_path := $(proc_dir)/crt0_$(PROC).c
  _crt0_family_path := $(family_dir)/crt0_$(FAMILY).c
  _crt0_arch_path := $(arch_dir)/crt0_$(ARCH).c
  _crt0_proc_path_s := $(proc_dir)/crt0_$(PROC).S
  _crt0_family_path_s := $(family_dir)/crt0_$(FAMILY).S
  _crt0_arch_path_s := $(arch_dir)/crt0_$(ARCH).S

# first, check if there is a file for the specific processor
  ifneq "$(wildcard $(_crt0_proc_path))" ""
    CFILES += $(_crt0_proc_path)
  else ifneq "$(wildcard $(_crt0_proc_path_s))" ""
    ASFILES += $(_crt0_proc_path_s)
# else, check if there is a file for processor family
  else ifneq "$(wildcard $(_crt0_family_path))" ""
    CFILES += $(_crt0_family_path)
  else ifneq "$(wildcard $(_crt0_family_path_s))" ""
    ASFILES += $(_crt0_family_path_s)
# else, check if there is a file for the arch
  else ifneq "$(wildcard $(_crt0_arch_path))" ""
    CFILES += $(_crt0_arch_path)
  else ifneq "$(wildcard $(_crt0_arch_path_s))" ""
    CFILES += $(_crt0_arch_path_s)
  else
    $(error Cannot find entry file "$(_crt0_proc_path)", "$(_crt0_family_path)", or "$(_crt0_arch_path). If you provide your own entry file, define NO_CRT0 to avoid this error")
  endif
endif

# find cpu files
_cpu_proc_path := $(proc_dir)/cpu_$(PROC).c
_cpu_family_path := $(family_dir)/cpu_$(FAMILY).c
_cpu_arch_path := $(arch_dir)/cpu_$(ARCH).c
_cpu_proc_path_s := $(proc_dir)/cpu_$(PROC).S
_cpu_family_path_s := $(family_dir)/cpu_$(FAMILY).S
_cpu_arch_path_s := $(arch_dir)/cpu_$(ARCH).S

# check if there is a file for the specific processor
ifneq "$(wildcard $(_cpu_proc_path))" ""
  CFILES += $(_cpu_proc_path)
else ifneq "$(wildcard $(_cpu_proc_path_s))" ""
  ASFILES += $(_cpu_proc_path_s)
endif
# check if there is a file for processor family
ifneq "$(wildcard $(_cpu_family_path))" ""
  CFILES += $(_cpu_family_path)
else ifneq "$(wildcard $(_cpu_family_path_s))" ""
  ASFILES += $(_cpu_family_path_s)
endif
#  check if there is a file for the arch
ifneq "$(wildcard $(_cpu_arch_path))" ""
  CFILES += $(_cpu_arch_path)
else ifneq "$(wildcard $(_cpu_arch_path_s))" ""
  CFILES += $(_cpu_arch_path_s)
endif
