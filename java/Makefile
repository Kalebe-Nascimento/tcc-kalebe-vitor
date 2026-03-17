JAVAC = javac
MPJ_HOME ?= /opt/mpj

SEQ_SRC = sequential/MedianFilter.java sequential/ConnectedComponents.java
PAR_SRC = parallel/MedianFilterMPI.java parallel/ConnectedComponentsMPI.java

SEQ_CLASSES = sequential/MedianFilter.class sequential/ConnectedComponents.class
PAR_CLASSES = parallel/MedianFilterMPI.class parallel/ConnectedComponentsMPI.class

.PHONY: all sequential parallel clean

all: sequential parallel

sequential: $(SEQ_CLASSES)

parallel: $(PAR_CLASSES)

sequential/%.class: sequential/%.java
	$(JAVAC) -sourcepath . $<

parallel/%.class: parallel/%.java
	$(JAVAC) -cp "$(MPJ_HOME)/lib/mpj.jar:." -sourcepath . $<

clean:
	rm -f sequential/*.class parallel/*.class
