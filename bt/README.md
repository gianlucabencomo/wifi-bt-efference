# Bluetooth for RV1126B

Use this command (Host Interface Configuration) to check bluetooth connection and hardware configuration
```
hciconfig -a
```
Use this command to look at the process status (ps) for bluetooth:
``` 
ps | grep bluetoothd
```
If this returns nothing, that means `BlueZ` is not installed in the file system or the boot failed. On the RV1126B from Luckfox, it should already be there.

## Device Names

Every bluetooth chip is imprinted with a globally unique 48-bit address. The right naming convention for our devices will be a projection of this naming address through hashing, ideally without collisons or a low probability of collision, to an `unsigned short int` 16-bits with a name that pops up like `M1 - 00035`.