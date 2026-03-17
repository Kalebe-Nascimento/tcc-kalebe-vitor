CXX = g++
MPICXX = mpicxx
CXXFLAGS = -O2 -std=c++17
MPIFLAGS = -O2 -std=c++17

SEQ_DIR = sequential
PAR_DIR = parallel

SEQ_TARGETS = $(SEQ_DIR)/median_filter $(SEQ_DIR)/connected_components
PAR_TARGETS = $(PAR_DIR)/median_filter_mpi $(PAR_DIR)/connected_components_mpi

.PHONY: all sequential parallel clean

all: sequential parallel

sequential: $(SEQ_TARGETS)

parallel: $(PAR_TARGETS)

$(SEQ_DIR)/median_filter: $(SEQ_DIR)/median_filter.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(SEQ_DIR)/connected_components: $(SEQ_DIR)/connected_components.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(PAR_DIR)/median_filter_mpi: $(PAR_DIR)/median_filter_mpi.cpp
	$(MPICXX) $(MPIFLAGS) -o $@ $<

$(PAR_DIR)/connected_components_mpi: $(PAR_DIR)/connected_components_mpi.cpp
	$(MPICXX) $(MPIFLAGS) -o $@ $<

clean:
	rm -f $(SEQ_TARGETS) $(PAR_TARGETS)
