MAIN=mitm
EXCLUDE_DIRS=Debug Release
$(MAIN):$(INTERNAL_OBJ)
	g++ -o $(MAIN) $(INTERNAL_OBJ) -lssl -lcrypto -lboost_system -lboost_thread 
	strip $(MAIN)

CLEAN_FILES+= $(MAIN)
