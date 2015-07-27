"Frequently Asked Questions about CCDK
====
- <B>What is the Shinra CCDK?</B><BR>
The Shinra CCDK, or Community Cloud Development Kit, is a toolkit that is designed to allow small developers and hobbyists to create their own cloud-powered games.
<BR><BR>
- <B>What do I need to do to get started with the CCDK?</B><BR>
To make a cloud game with the CCDK, you will need a PC running Windows 7, DirectX11, and the development environment of your choice (VS 2012/2013 have been tested). To create cloud games that utilize N:N architecture using the included VCE (Virtual Community Engine) middleware, a Linux virtual machine is required.
<BR><BR>
- <B>Where can I get the CCDK?</B><BR>
The CCDK will be distributed, updated, and supported via GitHub.
<BR><BR>
- <B>When is the CCDK coming out?</B><BR>
The initial version will be released on May 15. This version will only be supported in Japanese. A full English version will be released later in the year.
<BR><BR>
- <B>How much do I need to know about network programming to make a CCDK game?</B><BR>
Network programming knowledge is not required to make games under the 1:1 model or 1:N architectures. Running under the N:N architecture requires LAN/multiplayer programming knowledge, but this can be made easier with a standard sync server or by using the VCE (Virtual Community Engine) library.
<BR><BR>
- <B>What can't I do with the CCDK?</B><BR>
Heavy compute usage (for example, trying to use multiple GPUs linked in tandem) may still be limited in the initial stages of development; tuning the remote renderer itself may also be unavailable. Our technical support team can provide you with more information.
<BR><BR>
- <B>Do I need to be a subscriber to the Shinra service and/or have a fiber connection in order to make a CCDK-powered game?</B><BR>
Neither are required. You can develop your game using a local machine and local network.
<BR><BR>
- <B>If I make a game on Shinra, does it have to take advantage of sharing (or multiplayer)?</B><BR>
While there's nothing to stop you from making a single-player experience with the CCDK, sharing data by running games inside the same data center is a unique feature of the Shinra platform, and can result in interesting new multiplayer experiences.
<BR><BR>
- <B>Are there setup fees associated with becoming a Shinra developer?</B><BR>
No. However, the developer is responsible for providing their own game development environment.
<BR><BR>
- <B>Is ""early access"" publishing available on the Shinra platform?</B><BR>
Shinra is evaluating the possibilities of pre-release premium content.
<BR><BR>
- <B>Can Shinra games power virtual reality and augmented reality experiences?</B><BR>
Shinra's remote rendering technology can benefit a number of emerging technologies, and we are welcome to discussions with potential partners in these fields.
<BR><BR>
- <B>I am a game developer with a concept for a game. Who do I talk with?</B><BR>
Shinra is not a developer or publisher; however, we encourage self-publishing on our platform. AAA developers should contact DevUS@shinra.com, while Indie Developers should contact ccdk-en@shinra.com.
<BR><BR>
- <B>I am a student who is interested in the system. How do I get started?</B><BR>
The free Shinra CCDK, or Community Cloud Development Kit, is the best place to get started. Learn more by visiting tech.shinra.com.
<BR><BR>
- <B>Does every game on the Shinra platform run on the CCDK?</B><BR>
The CCDK is designed for smaller teams or individual developers, while larger teams can choose to use more demanding development environments.
<BR><BR>
- <B>How are the GPU and CPU servers connected to each other?</B><BR>
Both the production and development environments utilize a fiber channel for connection.
<BR><BR>
- <B>Am I able to take advantage of the client's GPU as well?</B><BR>
One of the benefits of Shinra is that there are no per-game installs on the client side; everything is kept inside the data center and transported by video stream.
<BR><BR>
- <B>For 1:1: 1:N, and N:N, all input is taken from the client PC side, but how do you handle an environment with latency?</B><BR>
In Japan, we are able to utilize NGNs to ensure the network environment is optimal. We're aiming for a maximum of 30ms to 40ms latency. Our rendering system already reduces the latency loop in half, making it feel seamless. 
<BR><BR>
- <B>How many players can each server realistically handle, and how many servers will be in each data center?</B><BR>
The number of supported players is dependent on the architecture used. Games that use the 1:N architecture are expected to scale to, for example, 24 players, while N:N architecture titles can theoretically be scaled into the hundreds (or beyond).
<BR><BR>
- <B>How will server maintenance and updating work? Will this ever affect the end user?</B><BR>
Developers will have the ability to work with Shinra in order to easily push updates across the platform. This process is designed to be as transparent to the end user as possible, though the end experience is likely to vary by title and game architecture.
<BR><BR>
- <B>Are hacking, cheating, and piracy a concern on Shinra's platform?</B><BR>
Because each gameplay session is streamed from a remote data center, no actual game code is stored on the user's hardware. This makes these issues less of a concern, but they are still taken seriously by Shinra.
<BR><BR>
- <B>What kind of architecture do I use to build a cloud game?</B><BR>
You can read a blog post [here] about the different types of architectures that run on Shinra, which we classify as 1:1, 1:N, and N:N. [https://tech.shinra.com/us/shinra-multiplayer-part1]
<BR><BR>
- <B>What is VCE, and how do I use it?</B><BR>
VCE (or Virtual Community Engine) is a library written in C that performs high-speed networking via TCP, and is very useful when writing Shinra N:N games. It runs under Windows, MacOS X and Linux, and was used as the networking base for the Shinra sample game Space Sweeper. VCE will be released for free, and is intended to be used with the CCDK.
<BR><BR>
- <B>Can I make a CCDK game under 1:1, 1:N, or N:N?</B><BR>
Yes. The CCDK does not limit your use of any game architecture.
<BR><BR>
- <B>What kind of middleware can I use to make a CCDK game?</B><BR>
Shinra emulates the Windows platform running Direct X11; however, licenses and other restrictions for streaming are the responsibility of the developer. We are currently preparing a list of tested middleware.
<BR><BR>
- <B>What language can I write my game in?</B><BR>
The CCDK sample programs (such as Space Sweeper) are written in C++, and the Shinra API is available as a C++ API. As long as you can load DLLs and run on the Windows platform, any language is compatible. We are working on a list of compatible programming languages.
<BR><BR>
- <B>Is it possible to use my own backend server that doesn't exist inside a Shinra data center?</B><BR>
Yes, there are currently no restrictions on where your backend server lives.
<BR><BR>
- <B>Can I use code that is specific to a certain type of GPU?</B><BR>
When programming for deployment on Shinra's platform, code should be GPU-agnostic. This is because hardware specifications may slightly change between data centers.
<BR><BR>
- <B>Is Shinra compatible with common engines such as Unity, Unreal, and GameMaker?</B><BR>
For 1:1 or N:N architectures any engine or toolkit compatible with Windows and DirectX11 will run on the Shinra system. For 1:N architectures, special engine support is required for graphics, sounds, and input. Further, each engine may handle streaming rights differently in its user agreement, so licenses and other restrictions for streaming are the responsibility of the developer.
<BR><BR>
- <B>What multiplayer networking libraries are supported for N:N games?</B><BR>
There are no specific limitations on networking libraries, as long as you are able to run multiple instances of the game on the same machine, and you don't use UDP multicast. VCE (Virtual Community Engine) is fully supported.
<BR><BR>
- <B>What controllers are supported by the Shinra platform?</B><BR>
The Shinra platform currently treats a traditional double-analog stick as its standard input device. Keyboard and mouse input are also supported. If other devices are required for a title, they will be evaluated on a case-by-case basis.
<BR><BR>
- <B>Can my game support UGC (user-generated content) or mods?</B><BR>
Though the Shinra platform is based on remote rendering, there are definite possibilities for the implementation of UGC inside titles.
<BR><BR>
- <B>How do I sell a game that I've written with the CCDK?</B><BR>
Our goal is to support independent developers who make use of the CCDK, and we'll be addressing the business side of CCDK development in a future update to the Q&A section.
<BR><BR>
- <B>Can I self-publish a game on the Shinra platform?</B><BR>
Yes! The Shinra platform is open to self-publishing.
<BR><BR>
- <B>What is the submission process like for getting onto the platform?</B><BR>
Shinra games should undergo extensive QA before submission, as they are subject to a certification process.
<BR><BR>
- <B>Will we be able to run online technical alphas of our games with consumers?</B><BR>
Shinra has long-term plans to allow its developers, from Prototype Accelerator partners to CCDK developers, to deploy in private and/or public test environments.
<BR><BR>
- <B>Are Shinra titles subject to ratings board approval?</B><BR>
At this point, ratings certification is not required by the Shinra platform. However, all content will be held to Shinra's Content Policy (https://tech.shinra.com/docs/ShinraContentPolicy.pdf).
<BR><BR>
- <B>Can I publish older catalog titles that I've developed to Shinra?</B><BR>
Shinra is always looking to feature quality content; older titles will be evaluated on a case-by-case basis.
<BR><BR>
- <B>Do Shinra titles require platform exclusivity?</B><BR>
No, platform exclusivity is not required. However, games that are specifically written for the Shinra platform are likely to be so unique in their architecture that it's unlikely that the same experience can be transferred to another platform.
<BR><BR>
- <B>Can I distribute or publish adult titles on the Shinra platform?</B><BR>
No. Shinra developers are held to its Content Policy, which does not permit adult titles. (https://tech.shinra.com/docs/ShinraContentPolicy.pdf)
<BR><BR>
- <B>How do you test a Shinra game locally?</B><BR>
A developer can run their game underneath the Shinra MCS (Minimum Cloud Set), which acts as a wrapper for the Shinra platform. The user can start their game and then stream it over their local network in order to test compatibility.
<BR><BR>
- <B>What does support refer to?</B><BR>
Support means responding to technical questions and escalating bug and feature requests to our dev team.
<BR><BR>

"