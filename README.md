# RemoteAutoBackup

## Project summary
The	project	aims	at	building	a	client-server	system	 that	performs	an	incremental	back-up	
of	 the	 content	 of	 a	 folder	 (and	 all	 its	 sub-folders)	 on	 the	 local	 computer	 onto	 a	 remote	
server.	Once	launched,	the	system	will	operate	as	a	background	service	keeping	the	content	
of	 the	 configured	 folder	 in	 sync	 with	 the	 remote	 copy:	 whenever	 the	 monitored	 content	
changes	 (new	 files	 are	 added,	 existing	 ones	 are	 changed	 and/or	 removed),	 	 suitable	
command	will	 be	 sent	across	 the	 network	in	 order	 to	 replicate	 the	 changes	 on	 the	 other	
side.	 In	case	 of	 transient	errors	 (network	portioning	 or	 failure	 of	 the	 remote	 server),	 the	
system	will	 keep	 on	monitoring	 the	evolution	 of	 the	local	 folder	and	 try	 to	 sync	again	as	
soon	as	the	transient	error	condition	vanishes).	

## Required Background and Working Environment
Knowledge	 of	 the	C++17	general	abstractions	and	 of	 the	C++	Standard	Template	Library.
Knowledge	of	concurrency,	synchronization	and	background	processing.
The	 system	 will	 be	 developed	 using	 third	 party	 libraries	 (e.g.,	 boost.asio)	 in	 order	 to	
support	deployment	on	several	platforms.	

## Problem definition

### Overall	architecture
The	system	consists	of	two	different	modules	(the	client	and	the	server)	that	interact	via	a	
TCP	socket	connection.
A	dialogue	takes	place	along	this	connection	allowing	the	two	parties	to	understand	eachother.	 The	 set	 of	 possible	messages,	 their	 encoding	 and	 their	meaning	 represents	 the	 so	
called	 “application-level	 protocol”	 which	 should	 be	 defined	 first,	 when	 approaching	 this	
kind	of	problems.
Its	content	stems	from	the	overall	requirements	of	the	system	and	should	allow	a	suitable	
representation	 for	 the	envisaged	entities	which	can	be	part	of	 the	conversation	 (amongst	
the	 others,	 files	 – comprising	 their	 name,	 their	 content	 and	 their	 metadata,	 folders,	
commands,	 responses,	 error	 indications,	 …).	 The	 set	 of	 possible	 requests	 and	 the	
corresponding	 responses	 should	 be defined	 (and	 probably,	 refined	 along	 the	 project,	 as	
more	details	emerge).
Moreover,	each	exchanged	message	should	also	be	related	with	 the	implications	it	has	on	
the	 receiving	 party,	 that	 should	 be	 coded	 taking	 such	 an	 implication	 as	 a	 mandatory	
requirement.	
In	 order	 to	 remain	 synced,	 both	 the	 client	 and	 the	 server	 need	 to	 have	 an	 image	 of	 the	
monitored	 files	 as	 they	 are	 stored	in	 the	 respective	 file	 system.	 The	 application	 protocol	
should	offer	some	kind	of	“probe	command”	that	allows	the	client	to	verify	whether	or	not	
the	server	already	has	a	copy	of	a	given	 file	or	 folder.	Since	 the	 filename	and	size	are	not	
enough	 to	 proof	 that	 the	 server	 has	 the	 same	 copy	 as	 the	 client,	 a	 (robust)	 checksum	 or	
hash	should	be	computed	per	each	 file	and	sent	 to	 the	server.	Since	 the	server	may	need	
some	time	to	compute	the	answer,	the	protocol	should	probably	operate	in	asynchronous	
mode,	 i.e.,	 without	 requiring	 an	 immediate	 response	 of	 the	 server	 before	 issuing	 a	 new	
request	to	it.	

### Client side
The	client	side	is	in	charge	of	continuously	monitoring	a	specific	folder	that	can	be	specified	
in	any	reasonable	way	(command	line	parameter,	configuration	file,	environment	
variable…)	and	check	that	all	the	contents	are	in	sync	with	the	sever	side.	To	perform	this	
operation,	it	can	rely	on	the	filesystem	class	provided	with	the	C++17	library	or	the	Boost	
one	(https://www.boost.org/doc/libs/1_73_0/libs/filesystem/doc/index.htm).	Whenever	
a	discrepancy	is	found,	the	local	corresponding	entry	should	be	marked	as	invalid	and	
some	arrangements	should	be	taken	to	transfer	the	(updated)	file	to	the	server.	Some	
indications	on	how	to	create	a	file	system	watcher	can	be	found	here	
(https://solarianprogrammer.com/2019/01/13/cpp-17-filesystem-write-file-watchermonitor/)

### Server side
The	 server	 side	 is	 responsible	 of	 listening	 on	 socket	 connection	 and	 accept	 connection	
requests	 from	 clients.	 The	 server	 should	 be	 designed	in	 order	 to	manage	more	 than	 one	
client	 and	 have	 separate	 backup	 folders	 for	 each	 of	 them.	 When	 the	 communication	
channel	is	setup,	a	check	of	the	client	identity	will	be	performed	(by	exchanging	a	name	and	
some	 form	 of	 proof	 of	 identity)	 in	 order	 to	 associate	 the	 connection	 to	 the	 proper	
configuration	 parameters.	 Per	 each	 existing	 connection,	 incoming	 messages	 will	 be	
evaluated	 in	 the	 order	 they	 arrive	 and	 a	 suitable	 response	 will	 be	 generated,	 either	
containing	the	requested	information	or	an	error	code.	In	case	of	success,	any	implication	
of	the	command	will	be	guaranteed.	In	case	of	error,	no	change	will	happen	on	the	server	
side.	
Communication	between	the	client	and	the	server	can be	based	on	the	Boost	ASIO	library	
(https://www.boost.org/doc/libs/1_73_0/doc/html/boost_asio.html)	 or	 any	 other	
suitable	one.