util.d: ../../include/v3FileUtil.h ../../include/v3HashUtil.h ../../include/v3IntType.h ../../include/v3Msg.h ../../include/v3NumUtil.h ../../include/v3StrUtil.h ../../include/v3Usage.h 
../../include/v3FileUtil.h: v3FileUtil.h
	@rm -f ../../include/v3FileUtil.h
	@ln -fs ../src/util/v3FileUtil.h ../../include/v3FileUtil.h
../../include/v3HashUtil.h: v3HashUtil.h
	@rm -f ../../include/v3HashUtil.h
	@ln -fs ../src/util/v3HashUtil.h ../../include/v3HashUtil.h
../../include/v3IntType.h: v3IntType.h
	@rm -f ../../include/v3IntType.h
	@ln -fs ../src/util/v3IntType.h ../../include/v3IntType.h
../../include/v3Msg.h: v3Msg.h
	@rm -f ../../include/v3Msg.h
	@ln -fs ../src/util/v3Msg.h ../../include/v3Msg.h
../../include/v3NumUtil.h: v3NumUtil.h
	@rm -f ../../include/v3NumUtil.h
	@ln -fs ../src/util/v3NumUtil.h ../../include/v3NumUtil.h
../../include/v3StrUtil.h: v3StrUtil.h
	@rm -f ../../include/v3StrUtil.h
	@ln -fs ../src/util/v3StrUtil.h ../../include/v3StrUtil.h
../../include/v3Usage.h: v3Usage.h
	@rm -f ../../include/v3Usage.h
	@ln -fs ../src/util/v3Usage.h ../../include/v3Usage.h
