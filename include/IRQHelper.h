// This .h file generates the enumeration and name pairs. 
// To use it, #define IRQPAIRS as a list of _IRQ(enum,name) commands.
// Then include this header right below. It will expand into the enum{}
// and the const char* [] that SimAVR requires.

#define _IRQ(x,y) x, 
enum IRQ { 
    IRQPAIRS 
    COUNT 
};
#undef _IRQ

#define _IRQ(x,y) #y,
const char *_IRQNAMES[IRQ::COUNT] = { 
    IRQPAIRS 
    };