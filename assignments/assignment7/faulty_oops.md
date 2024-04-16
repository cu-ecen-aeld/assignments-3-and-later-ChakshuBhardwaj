root@qemuarm64:~# echo "hello_world" > /dev/faulty
[   97.180283] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[   97.184086] Mem abort info:
[   97.184596]   ESR = 0x0000000096000045
[   97.185290]   EC = 0x25: DABT (current EL), IL = 32 bits
[   97.187626]   SET = 0, FnV = 0
[   97.187824]   EA = 0, S1PTW = 0
[   97.188050]   FSC = 0x05: level 1 translation fault
[   97.188302] Data abort info:
[   97.188446]   ISV = 0, ISS = 0x00000045
[   97.188594]   CM = 0, WnR = 1
[   97.188802] user pgtable: 4k pages, 39-bit VAs, pgdp=00000000436f1000
[   97.188970] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[   97.189994] Internal error: Oops: 96000045 [#1] PREEMPT SMP
[   97.190388] Modules linked in: scull(O) hello(O) faulty(O)
[   97.190920] CPU: 3 PID: 347 Comm: sh Tainted: G           O      5.15.124-yocto-standard #1
[   97.191226] Hardware name: linux,dummy-virt (DT)
[   97.191615] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[   97.191910] pc : faulty_write+0x18/0x20 [faulty]
[   97.192755] lr : vfs_write+0xf8/0x29c
[   97.193162] sp : ffffffc00b383d80
[   97.193247] x29: ffffffc00b383d80 x28: ffffff8002018000 x27: 0000000000000000
[   97.193482] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[   97.193648] x23: 0000000000000000 x22: ffffffc00b383dc0 x21: 0000005594d57030
[   97.193810] x20: ffffff80036fe100 x19: 000000000000000c x18: 0000000000000000
[   97.193968] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[   97.194124] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[   97.194444] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008268e3c
[   97.194642] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[   97.194802] x5 : 0000000000000001 x4 : ffffffc000b70000 x3 : ffffffc00b383dc0
[   97.194958] x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
[   97.195255] Call trace:
[   97.195358]  faulty_write+0x18/0x20 [faulty]
[   97.195551]  ksys_write+0x74/0x10c
[   97.195701]  __arm64_sys_write+0x24/0x30
[   97.195829]  invoke_syscall+0x5c/0x130
[   97.195967]  el0_svc_common.constprop.0+0x4c/0x100
[   97.196069]  do_el0_svc+0x4c/0xb4
[   97.196148]  el0_svc+0x28/0x80
[   97.196232]  el0t_64_sync_handler+0xa4/0x130
[   97.196326]  el0t_64_sync+0x1a0/0x1a4
[   97.196637] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[   97.197107] ---[ end trace de6f4eddd2b02b93 ]---
Segmentation fault


The issue occured when we tried to write to the /dev/faulty device. This error shows that there is a segmentation fault at the faulty_write() function which occured due to null pointer dereference.
