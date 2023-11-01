// lut.h
static const f32 sine[2096] = {
0.000000f,0.104769f,0.208385f,0.309708f,0.407621f,0.501048f,0.588960f,
0.670389f,0.744439f,0.810296f,0.867233f,0.914625f,0.951950f,0.978797f,
0.994870f,0.999993f,0.994110f,0.977284f,0.949701f,0.911665f,0.863594f,
0.806019f,0.739571f,0.664983f,0.583075f,0.494750f,0.400979f,0.302794f,
0.201277f,0.097544f,-0.007262f,-0.111989f,-0.215483f,-0.316604f,-0.414242f,
-0.507320f,-0.594813f,-0.675760f,-0.749269f,-0.814530f,-0.870826f,-0.917537f,
-0.954149f,-0.980259f,-0.995579f,-0.999941f,-0.993296f,-0.975719f,-0.947402f,
-0.908657f,-0.859910f,-0.801699f,-0.734663f,-0.659541f,-0.577160f,-0.488426f,
-0.394315f,-0.295865f,-0.194158f,-0.090314f,0.014524f,0.119202f,0.222569f,
0.323485f,0.420841f,0.513565f,0.600635f,0.681095f,0.754058f,0.818722f,
0.874373f,0.920401f,0.956298f,0.981669f,0.996235f,0.999835f,0.992431f,
0.974102f,0.945052f,0.905600f,0.856180f,0.797337f,0.729717f,0.654065f,
0.571214f,0.482076f,0.387631f,0.288920f,0.187029f,0.083079f,-0.021785f,
-0.126410f,-0.229643f,-0.330349f,-0.427417f,-0.519783f,-0.606426f,-0.686394f,
-0.758808f,-0.822870f,-0.877874f,-0.923216f,-0.958396f,-0.983027f,-0.996838f,
-0.999677f,-0.991513f,-0.972435f,-0.942654f,-0.902496f,-0.852406f,-0.792933f,
-0.724732f,-0.648554f,-0.565238f,-0.475700f,-0.380927f,-0.281960f,-0.179890f,
-0.075839f,0.029045f,0.133610f,0.236705f,0.337194f,0.433972f,0.525972f,
0.612185f,0.691658f,0.763519f,0.826975f,0.881329f,0.925982f,0.960444f,
0.984334f,0.997389f,0.999466f,0.990542f,0.970716f,0.940205f,0.899345f,
0.848586f,0.788487f,0.719709f,0.643010f,0.559232f,0.469300f,0.374202f,
0.274985f,0.172741f,0.068597f,-0.036304f,-0.140804f,-0.243755f,-0.344022f,
-0.440503f,-0.532135f,-0.617911f,-0.696885f,-0.768188f,-0.831036f,-0.884737f,
-0.928700f,-0.962441f,-0.985588f,-0.997887f,-0.999202f,-0.989520f,-0.968946f,
-0.937707f,-0.896145f,-0.844721f,-0.784000f,-0.714649f,-0.637430f,-0.553197f,
-0.462875f,-0.367458f,-0.267994f,-0.165583f,-0.061349f,0.043560f,0.147989f,
0.250792f,0.350832f,0.447011f,0.538269f,0.623605f,0.702075f,0.772817f,
0.835053f,0.888099f,0.931369f,0.964387f,0.986790f,0.998333f,0.998886f,
0.988445f,0.967124f,0.935159f,0.892899f,0.840812f,0.779471f,0.709550f,
0.631819f,0.547132f,0.456425f,0.360694f,0.260992f,0.158417f,0.054099f,
-0.050814f,-0.155168f,-0.257816f,-0.357624f,-0.453496f,-0.544376f,-0.629264f,
-0.707228f,-0.777406f,-0.839027f,-0.891413f,-0.933988f,-0.966282f,-0.987941f,
-0.998725f,-0.998517f,-0.987318f,-0.965252f,-0.932561f,-0.889606f,-0.836859f,
-0.774901f,-0.704413f,-0.626173f,-0.541039f,-0.449951f,-0.353911f,-0.253975f,
-0.151242f,-0.046846f,0.058066f,0.162338f,0.264824f,0.364396f,0.459956f,
0.550454f,0.634892f,0.712344f,0.781953f,0.842956f,0.894681f,0.936558f,
0.968127f,0.989039f,0.999066f,0.998095f,0.986139f,0.963329f,0.929915f,
0.886266f,0.832861f,0.770290f,0.699240f,0.620494f,0.534918f,0.443453f,
0.347109f,0.246944f,0.144061f,0.039590f,-0.065315f,-0.169500f,-0.271820f,
-0.371150f,-0.466393f,-0.556502f,-0.640486f,-0.717421f,-0.786460f,-0.846841f,
-0.897901f,-0.939079f,-0.969920f,-0.990086f,-0.999353f,-0.997621f,-0.984908f,
-0.961354f,-0.927219f,-0.882879f,-0.828820f,-0.765638f,-0.694030f,-0.614782f,
-0.528768f,-0.436932f,-0.340289f,-0.239900f,-0.136870f,-0.032334f,0.072560f,
0.176653f,0.278802f,0.377882f,0.472805f,0.562522f,0.646047f,0.722461f,
0.790923f,0.850680f,0.901074f,0.941551f,0.971663f,0.991080f,0.999588f,
0.997094f,0.983625f,0.959330f,0.924476f,0.879444f,0.824734f,0.760946f,
0.688783f,0.609038f,0.522590f,0.430390f,0.333452f,0.232845f,0.129670f,
0.025073f,-0.079801f,-0.183797f,-0.285769f,-0.384596f,-0.479190f,-0.568510f,
-0.651572f,-0.727465f,-0.795348f,-0.854477f,-0.904200f,-0.943972f,-0.973353f,
-0.992021f,-0.999770f,-0.996514f,-0.982290f,-0.957254f,-0.921682f,-0.875965f,
-0.820606f,-0.756215f,-0.683501f,-0.603263f,-0.516383f,-0.423821f,-0.326595f,
-0.225774f,-0.122468f,-0.017814f,0.087037f,0.190929f,0.292719f,0.391292f,
0.485554f,0.574471f,0.657065f,0.732427f,0.799728f,0.858226f,0.907278f,
0.946343f,0.974993f,0.992911f,0.999899f,0.995882f,0.980904f,0.955129f,
0.918841f,0.872439f,0.816433f,0.751442f,0.678180f,0.597454f,0.510151f,
0.417234f,0.319724f,0.218695f,0.115259f,0.010550f,-0.094271f,-0.198055f,
-0.299658f,-0.397963f,-0.491888f,-0.580399f,-0.662521f,-0.737351f,-0.804068f,
-0.861932f,-0.910309f,-0.948666f,-0.976581f,-0.993748f,-0.999976f,-0.995198f,
-0.979466f,-0.952952f,-0.915950f,-0.868866f,-0.812219f,-0.746632f,-0.672827f,
-0.591616f,-0.503893f,-0.410621f,-0.312832f,-0.211600f,-0.108040f,-0.003289f,
0.101497f,0.205166f,0.306577f,0.404613f,0.498200f,0.586299f,0.667945f,
0.742239f,0.808364f,0.865590f,0.913290f,0.950937f,0.978118f,0.994532f,
1.000000f,0.994461f,0.977975f,0.950726f,0.913012f,0.865249f,0.807962f,
0.741782f,0.667435f,0.585744f,0.497605f,0.403990f,0.305928f,0.204499f,
0.100819f,-0.003971f,-0.108721f,-0.212270f,-0.313483f,-0.411246f,-0.504482f,
-0.592165f,-0.673331f,-0.747085f,-0.812616f,-0.869205f,-0.916225f,-0.953160f,
-0.979603f,-0.995264f,-0.999971f,-0.993671f,-0.976434f,-0.948450f,-0.910025f,
-0.861584f,-0.803660f,-0.736891f,-0.662010f,-0.579843f,-0.491294f,-0.397338f,
-0.299004f,-0.197382f,-0.093589f,0.011235f,0.115936f,0.219360f,0.320370f,
0.417853f,0.510738f,0.598004f,0.678684f,0.751894f,0.816829f,0.872772f,
0.919110f,0.955331f,0.981036f,0.995944f,0.999889f,0.992829f,0.974841f,
0.946123f,0.906991f,0.857876f,0.799318f,0.731963f,0.656551f,0.573910f,
0.484954f,0.390661f,0.292067f,0.190259f,0.086357f,-0.018495f,-0.123144f,
-0.226442f,-0.327243f,-0.424442f,-0.516970f,-0.603807f,-0.683998f,-0.756661f,
-0.820996f,-0.876293f,-0.921948f,-0.957452f,-0.982418f,-0.996571f,-0.999755f,
-0.991935f,-0.973197f,-0.943747f,-0.903909f,-0.854120f,-0.794932f,-0.726994f,
-0.651054f,-0.567949f,-0.478592f,-0.383967f,-0.285116f,-0.183127f,-0.079118f,
0.025758f,0.130350f,0.233508f,0.334095f,0.431005f,0.523171f,0.609579f,
0.689280f,0.761391f,0.825121f,0.879770f,0.924735f,0.959522f,0.983748f,
0.997146f,0.999568f,0.990988f,0.971500f,0.941320f,0.900778f,0.850322f,
0.790506f,0.721990f,0.645526f,0.561958f,0.472201f,0.377249f,0.278145f,
0.175980f,0.071878f,-0.033015f,-0.137545f,-0.240561f,-0.340930f,-0.437549f,
-0.529348f,-0.615321f,-0.694522f,-0.766078f,-0.829202f,-0.883199f,-0.927475f,
-0.961543f,-0.985026f,-0.997668f,-0.999328f,-0.989989f,-0.969754f,-0.938844f,
-0.897601f,-0.846478f,-0.786036f,-0.716944f,-0.639961f,-0.555934f,-0.465788f,
-0.370515f,-0.271164f,-0.168828f,-0.064634f,0.040275f,0.144737f,0.247606f,
0.347750f,0.444066f,0.535494f,0.621029f,0.699727f,0.770727f,0.833240f,
0.886582f,0.930166f,0.963511f,0.986252f,0.998137f,0.999036f,0.988938f,
0.967956f,0.936318f,0.894377f,0.842588f,0.781529f,0.711864f,0.634362f,
0.549884f,0.459347f,0.363761f,0.264165f,0.161667f,0.057383f,-0.047532f,
-0.151917f,-0.254638f,-0.354548f,-0.450563f,-0.541612f,-0.626706f,-0.704896f,
-0.775333f,-0.837235f,-0.889918f,-0.932809f,-0.965430f,-0.987427f,-0.998554f,
-0.998691f,-0.987835f,-0.966106f,-0.933742f,-0.891104f,-0.838654f,-0.776977f,
-0.706743f,-0.628736f,-0.543802f,-0.452889f,-0.356985f,-0.257151f,-0.154494f,
-0.050129f,0.054780f,0.159094f,0.261649f,0.361331f,0.457030f,0.547704f,
0.632350f,0.710030f,0.779900f,0.841181f,0.893207f,0.935399f,0.967298f,
0.988548f,0.998918f,0.998293f,0.986680f,0.964205f,0.931120f,0.887784f,
0.834679f,0.772383f,0.701591f,0.623070f,0.537691f,0.446401f,0.350190f,
0.250132f,0.147313f,0.042880f,-0.062032f,-0.166261f,-0.268653f,-0.368095f,
-0.463479f,-0.553768f,-0.637955f,-0.715127f,-0.784422f,-0.845087f,-0.896450f,
-0.937943f,-0.969115f,-0.989618f,-0.999230f,-0.997843f,-0.985472f,-0.962256f,
-0.928446f,-0.884417f,-0.830656f,-0.767749f,-0.696396f,-0.617372f,-0.531559f,
-0.439889f,-0.343383f,-0.243092f,-0.140124f,-0.035622f,0.069281f,0.173413f,
0.275643f,0.374833f,0.469904f,0.559796f,0.643533f,0.720186f,0.788906f,
0.848949f,0.899643f,0.940438f,0.970879f,0.990636f,0.999488f,0.997339f,
0.984212f,0.960253f,0.925723f,0.881007f,0.826590f,0.763079f,0.691164f,
0.611647f,0.525392f,0.433353f,0.336552f,0.236039f,0.132935f,0.028361f,
-0.076518f,-0.180562f,-0.282612f,-0.381558f,-0.476304f,-0.565800f,-0.649076f,
-0.725201f,-0.793350f,-0.852761f,-0.902791f,-0.942882f,-0.972594f,-0.991602f,
-0.999694f,-0.996783f,-0.982902f,-0.958200f,-0.922954f,-0.877547f,-0.822479f,
-0.758364f,-0.685896f,-0.605884f,-0.519197f,-0.426802f,-0.329702f,-0.228981f,
-0.125732f,-0.021099f,0.083759f,0.187703f,0.289573f,0.388263f,0.482672f,
0.571775f,0.654579f,0.730184f,0.797751f,0.856533f,0.905891f,0.945275f,
0.974257f,0.992514f,0.999847f,0.996176f,0.981538f,0.956097f,0.920134f,
0.874040f,0.818330f,0.753608f,0.680597f,0.600088f,0.512981f,0.420221f,
0.322835f,0.221903f,0.118522f,0.013843f,-0.090996f,-0.194825f,-0.296518f,
-0.394940f,-0.489021f,-0.577720f,-0.660054f,-0.735128f,-0.802106f,-0.860259f,
-0.908941f,-0.947620f,-0.975869f,-0.993375f,-0.999948f,-0.995515f,-0.980123f,
-0.953945f,-0.917265f,-0.870492f,-0.814133f,-0.748813f,-0.675257f,-0.594262f,
-0.506732f,-0.413618f,-0.315959f,-0.214814f,-0.111313f,-0.006579f,0.098228f,
0.201945f,0.303448f,0.401603f,0.495345f,0.583628f,0.665493f,0.740028f,
0.806423f,0.863941f,0.911945f,0.949916f,0.977428f,0.994184f,0.999996f,
0.994801f,0.978658f,0.951741f,0.914347f,0.866893f,0.809894f,0.743984f,
0.669881f,0.588410f,0.500456f,0.407000f,0.309058f,0.207714f,0.104091f,
-0.000686f,-0.105447f,-0.209055f,-0.310355f,-0.408245f,-0.501636f,-0.589512f,
-0.670898f,-0.744895f,-0.810697f,-0.867572f,-0.914902f,-0.952158f,-0.978937f,
-0.994939f,-0.999991f,-0.994035f,-0.977139f,-0.949486f,-0.911385f,-0.863249f,
-0.805616f,-0.739110f,-0.664469f,-0.582520f,-0.494153f,-0.400354f,-0.302141f,
-0.200610f,-0.096863f,0.007942f,0.112668f,0.216153f,0.317252f,0.414866f,
0.507907f,0.595364f,0.676262f,0.749722f,0.814924f,0.871162f,0.917810f,
0.954353f,0.980394f,0.995643f,0.999933f,0.993217f,0.975569f,0.947184f,
0.908371f,0.859559f,0.801291f,0.734198f,0.659029f,0.576600f,0.487832f,
0.393687f,0.295216f,0.193488f,0.089630f,-0.015206f,-0.119883f,-0.223233f,
-0.324133f,-0.421458f,-0.514151f,-0.601179f,-0.681595f,-0.754509f,-0.819113f,
-0.874706f,-0.920667f,-0.956498f,-0.981798f,-0.996294f,-0.999823f,-0.992347f,
-0.973947f,-0.944829f,-0.905309f,-0.855828f,-0.796923f,-0.729252f,-0.653548f,
-0.570650f,-0.481477f,-0.386999f,-0.288267f,-0.186356f,-0.082400f,0.022469f,
0.127084f,0.230308f,0.330996f,0.428035f,0.520368f,0.606968f,0.686893f,
0.759252f,0.823259f,0.878200f,0.923478f,0.958592f,0.983152f,0.996892f,
0.999659f,0.991423f,0.972276f,0.942425f,0.902203f,0.852048f,0.792514f,
0.724262f,0.648032f,0.564676f,0.475098f,0.380297f,0.281303f,0.179221f,
0.075158f,-0.029731f,-0.134287f,-0.237371f,-0.337835f,-0.434589f,-0.526551f,
-0.612725f,-0.692149f,-0.763959f,-0.827361f,-0.881651f,-0.926241f,-0.960633f,
-0.984454f,-0.997438f,-0.999443f,-0.990449f,-0.970551f,-0.939971f,-0.899046f,
-0.848223f,-0.788068f,-0.719234f,-0.642488f,-0.558665f,-0.468700f,-0.373568f,
-0.274325f,-0.172070f,-0.067913f,0.036984f,0.141482f,0.244414f,0.344664f,
0.441120f,0.532713f,0.618450f,0.697374f,0.768627f,0.831415f,0.885056f,
0.928952f,0.962626f,0.985704f,0.997931f,0.999175f,0.989421f,0.968776f,
0.937469f,0.895842f,0.844357f,0.783575f,0.714167f,0.636905f,0.552626f,
0.462270f,0.366820f,0.267339f,0.164909f,0.060671f,-0.044243f,-0.148669f,
-0.251452f,-0.351474f,-0.447621f,-0.538847f,-0.624136f,-0.702562f,-0.773248f,
-0.835429f,-0.888414f,-0.931617f,-0.964568f,-0.986901f,-0.998372f,-0.998854f,
-0.988341f,-0.966951f,-0.934916f,-0.892590f,-0.840443f,-0.779041f,-0.709069f,
-0.631287f,-0.546563f,-0.455816f,-0.360059f,-0.260332f,-0.157740f,-0.053418f,
0.051499f,0.155841f,0.258476f,0.358259f,0.454105f,0.544952f,0.629796f,
0.707713f,0.777834f,0.839400f,0.891722f,0.934232f,0.966457f,0.988047f,
0.998760f,0.998480f,0.987209f,0.965073f,0.932314f,0.889295f,0.836484f,
0.774471f,0.703928f,0.625637f,0.540465f,0.449338f,0.353273f,0.253312f,
0.150570f,0.046163f,-0.058745f,-0.163013f,-0.265487f,-0.365031f,-0.460565f,
-0.551023f,-0.635422f,-0.712821f,-0.782379f,-0.843322f,-0.894986f,-0.936799f,
-0.968297f,-0.989140f,-0.999095f,-0.998053f,-0.986026f,-0.963145f,-0.929665f,
-0.885949f,-0.832481f,-0.769855f,-0.698750f,-0.619959f,-0.534339f,-0.442844f,
-0.346467f,-0.246285f,-0.143384f,-0.038905f,0.065995f,0.170176f,0.272476f,
0.371785f,0.466994f,0.557070f,0.641008f,0.717897f,0.786883f,0.847204f,
0.898203f,0.939313f,0.970087f,0.990181f,0.999377f,0.997573f,0.984790f,
0.961165f,0.926964f,0.882557f,0.828439f,0.765198f,0.693540f,0.614243f,
0.528184f,0.436319f,0.339644f,0.239238f,0.136191f,0.031653f,-0.073242f,
-0.177322f,-0.279459f,-0.378519f,-0.473405f,-0.563088f,-0.646567f,-0.722935f,
-0.791341f,-0.851041f,-0.901369f,-0.941780f,-0.971824f,-0.991170f,-0.999607f,
-0.997042f,-0.983502f,-0.959137f,-0.924214f,-0.879121f,-0.824348f,-0.760501f,
-0.688288f,-0.608494f,-0.522009f,-0.429771f,-0.332810f,-0.232178f,-0.128998f,
-0.024391f,0.080485f,0.184467f,0.286426f,0.385226f,0.479792f,0.569070f,
0.652092f,0.727930f,0.795761f,0.854833f,0.904491f,0.944198f,0.973509f,
0.992107f,0.999784f,0.996457f,0.982162f,0.957057f,0.921416f,0.875636f,
0.820214f,0.755769f,0.683000f,0.602713f,0.515805f,0.423207f,0.325950f,
0.225106f,0.121783f,0.017136f,-0.087716f,-0.191602f,-0.293379f,-0.391912f,
-0.486146f,-0.575029f,-0.657582f,-0.732897f,-0.800135f,-0.858576f,-0.907566f,
-0.946566f,-0.975143f,-0.992991f,-0.999909f,-0.995820f,-0.980770f,-0.954928f,
-0.918572f,-0.872104f,-0.816037f,-0.750987f,-0.677682f,-0.596907f,-0.509562f,
-0.416607f,-0.319081f,-0.218029f,-0.114577f,-0.009864f,0.094957f,0.198719f,
0.300308f,0.398592f,0.492488f,0.580951f,0.663032f,0.737814f,0.804475f,
0.862281f,0.910589f,0.948881f,0.976728f,0.993824f,0.999981f,0.995131f,
0.979327f,0.952744f,0.915673f,0.868530f,0.811821f,0.746176f,0.672316f,
0.591069f,0.503304f,0.409999f,0.312181f,0.210927f,0.107365f,0.002608f,
-0.102179f,-0.205841f,-0.307222f,-0.405237f,-0.498791f,-0.586854f,-0.668458f,
-0.742693f,-0.808765f,-0.865934f,-0.913571f,-0.951147f,-0.978259f,-0.994603f,
-0.999999f,-0.994388f,-0.977834f,-0.950514f,-0.912732f,-0.864903f,-0.807562f,
-0.741325f,-0.666927f,-0.585188f,-0.497007f,-0.403370f,-0.305279f,-0.203828f,
-0.100133f,0.004649f,0.109395f,0.212937f,0.314134f,0.411874f,0.505067f,
0.592715f,0.673838f,0.747543f,0.813011f,0.869540f,0.916498f,0.953367f,
0.979741f,0.995330f,0.999966f,0.993594f,0.976285f,0.948235f,0.909743f,
0.861238f,0.803252f,0.736425f,0.661502f,0.579288f,0.490697f,0.396705f,
0.298360f,0.196718f,0.092910f,-0.011921f,-0.116621f,-0.220021f,-0.321016f,
-0.418476f,-0.511330f,-0.598544f,-0.679182f,-0.752344f,-0.817224f,-0.873109f,
-0.919377f,-0.955532f,-0.981169f,-0.996006f,-0.999879f,-0.992748f,-0.974689f,
-0.945900f,-0.906700f,-0.857527f,-0.798908f,-0.731496f,-0.656031f,-0.573357f,
-0.484361f,-0.390033f,-0.291412f,-0.189583f,-0.085682f,0.019177f,0.123825f,
0.227109f,0.327894f,0.425056f,0.517553f,0.604353f,0.684501f,0.757104f,
0.821385f,0.876624f,0.922213f,0.957651f,0.982545f,0.996628f,0.999740f,
0.991847f,0.973041f,0.943521f,0.903616f,0.853764f,0.794514f,0.726528f,
0.650537f,0.567384f,0.477986f,0.383341f,0.284462f,0.182452f,0.078434f,
-0.026447f,-0.131022f,-0.234171f,-0.334741f,-0.431627f,-0.523749f,-0.610119f,
-0.689773f,-0.761835f,-0.825511f,-0.880092f,-0.924995f,-0.959715f,-0.983872f,
-0.997197f,-0.999548f,-0.990896f,-0.971338f,-0.941087f,-0.900483f,-0.849963f,
-0.790086f,-0.721512f,-0.645008f,-0.561394f,-0.471599f,-0.376614f,-0.277483f,
-0.175313f,-0.071198f,0.033701f,0.138228f,0.241219f,0.341571f,0.438162f,
0.529930f,0.615865f,0.695009f,0.766516f,0.829585f,0.883522f,0.927728f,
0.961729f,0.985144f,0.997715f,0.999303f,0.989893f,0.969587f,0.938608f,
0.897297f,0.846117f,0.785617f,0.716469f,0.639434f,0.555361f,0.465188f,
0.369882f,0.270504f,0.168149f,0.063958f,-0.040952f,-0.145412f,-0.248270f,
-0.348396f,-0.444673f,-0.536070f,-0.621566f,-0.700220f,-0.771156f,-0.833615f,
-0.886898f,-0.930418f,-0.963696f,-0.986364f,-0.998179f,-0.999006f,-0.988836f,
-0.967785f,-0.936080f,-0.894070f,-0.842219f,-0.781097f,-0.711387f,-0.633838f,
-0.549311f,-0.458738f,-0.363130f,-0.263511f,-0.160991f,-0.056699f,0.048217f,
0.152588f,0.255293f,0.355189f,0.451175f,0.542182f,0.627234f,0.705382f,
0.775765f,0.837609f,0.890227f,0.933053f,0.965608f,0.987535f,0.998591f,
0.998656f,0.987729f,0.965929f,0.933497f,0.890796f,0.838284f,0.776545f,
0.706258f,0.628197f,0.543233f,0.452278f,0.356345f,0.256489f,0.153824f,
0.049452f,-0.055464f,-0.159770f,-0.262318f,-0.361963f,-0.457639f,-0.548278f,
-0.632881f,-0.710507f,-0.780324f,-0.841552f,-0.893516f,-0.935644f,-0.967470f,
-0.988651f,-0.998950f,-0.998252f,-0.986569f,-0.964025f,-0.930870f,-0.887468f,
-0.834297f,-0.771952f,-0.701102f,-0.622534f,-0.537113f,-0.445794f,-0.349555f,
-0.249468f,-0.146635f,-0.042188f,0.062708f,0.166930f,0.269313f,0.368733f,
0.464080f,0.554332f,0.638483f,0.715606f,0.784851f,0.845449f,0.896751f,
0.938181f,0.969284f,0.989715f,0.999256f,0.997797f,0.985355f,0.962067f,
0.928194f,0.884100f,0.830274f,0.767309f,0.695909f,0.616838f,0.530978f,
0.439273f,0.342732f,0.242434f,0.139453f,0.034936f,-0.069965f,-0.174080f,
-0.276295f,-0.375468f,-0.470509f,-0.560370f,-0.644051f,-0.720656f,-0.789328f,
-0.849311f,-0.899938f,-0.940668f,-0.971043f,-0.990729f,-0.999510f,-0.997290f,
-0.984092f,-0.960062f,-0.925464f,-0.880686f,-0.826208f,-0.762635f,-0.690668f,
-0.611098f,-0.524815f,-0.432742f,-0.335906f,-0.235372f,-0.132263f,-0.027683f,
0.077202f,0.181237f,0.283277f,0.382184f,0.476900f,0.566366f,0.649597f,
0.725668f,0.793762f,0.853119f,0.903085f,0.943111f,0.972751f,0.991689f,
0.999711f,0.996728f,0.982777f,0.958006f,0.922690f,0.877218f,0.822089f,
0.757921f,0.685402f,0.605338f,0.518611f,0.426175f,0.329062f,0.228313f,
0.125052f,0.020413f,-0.084435f,-0.188368f,-0.290229f,-0.388894f,-0.483279f,
-0.572331f,-0.655097f,-0.730652f,-0.798164f,-0.856883f,-0.906178f,-0.945499f,
-0.974411f,-0.992598f,-0.999859f,-0.996115f,-0.981407f,-0.955896f,-0.919868f,
-0.873711f,-0.817936f,-0.753158f,-0.680089f,-0.599546f,-0.512393f,-0.419599f,
-0.322186f,-0.221242f,-0.117848f,-0.013157f,0.091679f,0.195505f,0.297165f,
0.395570f,0.489619f,0.578280f,0.660563f,0.735587f,0.802515f,0.860609f,
0.909229f,0.947837f,0.976017f,0.993454f,0.999955f,0.995450f,0.979988f,
0.953739f,0.916992f,0.870150f,0.813740f,0.748364f,0.674751f,0.593710f,
0.506147f,0.413001f,0.315308f,0.214145f,0.110624f,0.005901f,-0.098902f,
-0.202617f,-0.304101f,-0.402224f,-0.495934f,-0.584185f,-0.666005f,-0.740494f,
-0.806823f,-0.864282f,-0.912226f,-0.950130f,-0.977571f,-0.994256f,-0.999997f,
-0.994731f,-0.978515f,-0.951532f,-0.914073f,-0.866551f,-0.809491f,-0.743531f,
-0.669377f,-0.587855f,-0.499862f,-0.406367f,-0.308413f,-0.207051f,-0.103409f,
0.001371f,0.106121f,0.209718f,0.311006f,0.408871f,0.502235f,0.590059f,
0.671401f,0.745352f,0.811098f,0.867909f,0.915175f,0.952368f,0.979077f,
0.995008f,0.999988f,0.993961f,0.976993f,0.949271f,0.911106f,0.862907f,
0.805209f,0.738648f,0.663957f,0.581969f,0.493564f,0.399726f,0.301487f,
0.199946f,0.096188f,-0.008628f,-0.113349f,-0.216823f,-0.317895f,-0.415483f,
-0.508497f,-0.595915f,-0.676772f,-0.750170f,-0.815322f,-0.871498f,-0.918082f,
-0.954556f,-0.980528f,-0.995706f,-0.999925f,-0.993137f,-0.975420f,-0.946964f,
-0.908084f,-0.859209f,-0.800885f,-0.733737f,-0.658513f,-0.576040f,-0.487226f,
-0.393063f,-0.294560f,-0.192815f,-0.088947f,0.015884f,0.120556f,0.223901f,
0.324781f,0.422087f,0.514733f,0.601726f,0.682097f,0.754959f,0.819502f,
0.875034f,0.920934f,0.956698f,0.981930f,0.996352f,0.999809f,0.992262f,
0.973791f,0.944607f,0.905021f,0.855474f,0.796509f,0.728777f,0.653034f,
0.570093f,0.480876f,0.386366f,0.287618f,0.185689f,0.081717f,-0.023155f,
-0.127772f,-0.230968f,-0.331636f,-0.428654f,-0.520954f,-0.607507f,-0.687385f,
-0.759698f,-0.823648f,-0.878531f,-0.923738f,-0.958785f,-0.983277f,-0.996946f,
-0.999641f,-0.991334f,-0.972115f,-0.942195f,-0.901904f,-0.851693f,-0.792100f,
-0.723789f,-0.647510f,-0.564116f,-0.474501f,-0.379663f,-0.280645f,-0.178539f,
-0.074482f,0.030409f,0.134966f,0.238037f,0.338473f,0.435199f,0.527134f,
0.613267f,0.692649f,0.764397f,0.827741f,0.881974f,0.926499f,0.960821f,
0.984573f,0.997487f,0.999420f,0.990353f,0.970388f,0.939739f,0.898746f,
0.847860f,0.787650f,0.718762f,0.641963f,0.558097f,0.468087f,0.372939f,
0.273673f,0.171394f,0.067229f,-0.037662f,-0.142153f,-0.245079f,-0.345307f,
-0.441735f,-0.533287f,-0.618983f,-0.697865f,-0.769065f,-0.831791f,-0.885371f,
-0.929206f,-0.962812f,-0.985819f,-0.997974f,-0.999147f,-0.989321f,-0.968606f,
-0.937228f,-0.895540f,-0.843990f,-0.783149f,-0.713687f,-0.636382f,-0.552060f,
-0.461662f,-0.366182f,-0.266671f,-0.164240f,-0.059986f,0.044928f,0.149347f,
0.252108f,0.352109f,0.448234f,0.539425f,0.624678f,0.703044f,0.773683f,
0.835806f,0.888729f,0.931863f,0.964747f,0.987011f,0.998411f,0.998820f,
0.988238f,0.966776f,0.934673f,0.892281f,0.840075f,0.778616f,0.708586f,
0.630755f,0.545982f,0.455213f,0.359420f,0.259670f,0.157063f,0.052741f,
-0.052176f,-0.156519f,-0.259138f,-0.358906f,-0.454709f,-0.545521f,-0.630328f,
-0.708197f,-0.778260f,-0.839768f,-0.892032f,-0.934477f,-0.966635f,-0.988151f,
-0.998793f,-0.998442f,-0.987099f,-0.964896f,-0.932068f,-0.888981f,-0.836108f,
-0.774032f,-0.703446f,-0.625108f,-0.539888f,-0.448726f,-0.352638f,-0.252656f,
-0.149892f,-0.045478f,0.059437f,0.163682f,0.266140f,0.365670f,0.461173f,
0.551588f,0.635945f,0.713301f,0.782806f,0.843694f,0.895288f,0.937036f,
0.968469f,0.989241f,0.999123f,0.998010f,0.985912f,0.962960f,0.929409f,
0.885634f,0.832105f,0.769417f,0.698259f,0.619427f,0.533766f,0.442229f,
0.345824f,0.245613f,0.142713f,0.038227f,-0.066679f,-0.170851f,-0.273128f,
-0.372414f,-0.467600f,-0.557639f,-0.641540f,-0.718369f,-0.787301f,-0.847568f,
-0.898504f,-0.939545f,-0.970251f,-0.990276f,-0.999401f,-0.997525f,-0.984672f,
-0.960978f,-0.926706f,-0.882234f,-0.828059f,-0.764761f,-0.693046f,-0.613702f,
-0.527602f,-0.435709f,-0.339006f,-0.238572f,-0.135512f,-0.030960f,0.073918f,
0.177997f,0.280117f,0.379153f,0.474003f,0.563648f,0.647090f,0.723409f,
0.791764f,0.851396f,0.901666f,0.942011f,0.971986f,0.991260f,0.999626f,
0.996989f,0.983377f,0.958941f,0.923955f,0.878794f,0.823960f,0.760056f,
0.687796f,0.607956f,0.521424f,0.429152f,0.332156f,0.231518f,0.128318f,
0.023706f,-0.081168f,-0.185133f,-0.287076f,-0.385858f,-0.480393f,-0.569640f,
-0.652606f,-0.728400f,-0.796176f,-0.855188f,-0.904780f,-0.944421f,-0.973666f,
-0.992193f,-0.999799f,-0.996400f,-0.982034f,-0.956858f,-0.921149f,-0.875308f,
-0.819826f,-0.755320f,-0.682499f,-0.602166f,-0.515218f,-0.422586f,-0.325302f,
-0.224438f,-0.121118f,-0.016450f,};
