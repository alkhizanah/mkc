#ifndef EXCEPTIONS_H_ 
#define EXCEPTIONS_H_ 

#define ENABLE_EXCEPTIONS(stream) (stream).exceptions(std::ifstream::badbit)
#define PRINT_ENABLED_EXCEPTIONS                                   \
    {                                                              \
        std::ios_base::iostate exceptions = (in).exceptions();     \
        if (exceptions & std::ifstream::failbit) {                 \
            std::cout << "\nfailbit is enabled";                   \
        }                                                          \
        if (exceptions & std::ifstream::badbit) {                  \
            std::cout << "\nbadbit is enabled";                    \
        }                                                          \
        if (exceptions & std::ifstream::eofbit) {                  \
            std::cout << "\neofbit is enabled";                    \
        }                                                          \
    }
#endif
