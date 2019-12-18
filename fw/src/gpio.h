#define MODE_IN                 0x00
#define MODE_OUT                0x01
#define MODE_AF                 0x02
#define MODE_AN                 0x03
#define MODER(mode, pin)        ((mode) << (2 * (pin)))

#define PULL_NO                 0x00
#define PULL_UP                 0x01
#define PULL_DOWN               0x10
#define PUPDR(pull, pin)        ((pull) << (2 * (pin)))

#define OTYPE_PP                0x00
#define OTYPE_OD                0x01
#define OTYPER(otype, pin)      ((otype) << (pin))

#define AF0                     0x00
#define AF1                     0x01
#define AF2                     0x02
#define AF3                     0x03
#define AF4                     0x04
#define AF5                     0x05
#define AF6                     0x06
#define AF7                     0x07
#define AFR(af, pin)            ((af) << (4 * (((pin) % 8))))