APP			= hello5
STACK		= 1k
MALLOC		= 0k

include ../../app_make.txt

# 覆盖掉压缩规则, 让此app不压缩(此app过小, 压缩反而体积会变大)
$(APP).hrb : $(APP).org Makefile
	$(COPY) $< $@
	