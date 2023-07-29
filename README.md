# AdjustPrivileges
Adjust Process Privileges In Windows

权限提升是程序员的常见操作。  
权限提升是安全员的常用伎俩。  

权限提升是进程的所属用户的权限的使能动作。这是程序员的视角。  
权限提升是一个用户切换到另一个高权限的用户的行为。这是安全员的认识。  
真正的提权是直面权限的设计的攻击，其次是漏洞，最后是正规的授权。  

权限不是一个物主，是宿主。  
权限属于：  
1.CPU设定的权限，如：ring0，ring3等，限定指令的执行。  
2.操作系统的权限，归属于用户（以及各种对象），应用于进程。  
3.数据库等第三方应用的权限。这里不谈。  

作为一个驱动开发的老手，能否写一个供普通用户提权的驱动？  
这个问题，别人不问，你自己都不问问自己吗？  
特别是当你以普通的非管理员权限账户运行操作系统时，你没有想突破系统的限制的想法吗？  
没有漏洞，即使给你一个合法签名的驱动，甚至是你自己写的，你能做到吗？统治操作系统。  

起初的设计设想：  
1.建立一个任何用户都能打开的驱动对象（或者minifilter的通讯对象也行）。  
2.给打开的进程赋予高的权限，主要是修改TOKEN。  
3.没有第三步了。  
都这么简单。  

其实，也无需创建啥对象，各种（文件，网络，进程，线程，模块，注册表等）过滤里即可操作（赋予进程权限）。  

接下来是重点，都两个：  
1.The Security Descriptor Definition Language (SDDL)。  
2.TOKEN的操作。  
3.其他的都是辅助的。  

可能用到的函数：  
IoCreateDeviceSecure  
ZwAdjustPrivilegesToken  
ZwAdjustGroupsToken  
ZwSetInformationToken  
ZwDuplicateToken  
SeSetSecurityAttributesToken  
SeSetSecurityAttributesTokenEx  
RtlLengthSid  
RtlCopySid  
SecLookupAccountName  
SecLookupWellKnownSid  

另外一个话题：驱动安装启动等的设置是否可以用低权限。  
应该不可以，即使做到了，驱动的运行也有限制，可能会出问题。  

参考：  
Windows-driver-samples\general\cancel\sys\cancel.c  
https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/sddl-for-device-objects  
https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/4f4251cc-23b6-44b6-93ba-69688422cb06  
https://learn.microsoft.com/en-us/windows/win32/secauthz/security-descriptor-definition-language  

最后本程序无视UAC。  

经测试：Users组下的用户可以获取nt authority\system的权限。  

测试方法：  
1.登录到设定的低权限的用户去运行程序。  
2.runas /user:test C:\test.exe  
3.PsExec.exe -i -u "test" C:\test.exe  
