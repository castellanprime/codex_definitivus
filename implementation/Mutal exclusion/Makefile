centralized_srcs = centralized.cc
decentralized_srcs = decentralized.cc
token_ring_srcs = token_ring.cc

all: centralized decentralized token_ring

centralized: $(centralized_srcs)
	$(CXX) $(centralized_srcs) -o centralized

decentralized: $(decentralized_srcs)
	$(CXX) $(decentralized_srcs) -o decentralized

token_ring: $(token_ring_srcs)
	$(CXX) $(token_ring_srcs) -o token_ring

clean:
	rm centralized decentralized token_ring
