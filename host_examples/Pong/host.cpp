#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define BATCH_SIZE 1
#define OUT_SIZE 4

#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <CL/cl2.hpp>
#include "./block.h"

// Forward declaration of utility functions included at the end of this file
std::vector<cl::Device> get_xilinx_devices();
char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb);

// HBM Pseudo-channel(PC) requirements
#define MAX_HBM_PC_COUNT 16
#define PC_NAME(n) n | XCL_MEM_TOPOLOGY
const int pc[MAX_HBM_PC_COUNT] = {
    PC_NAME(0),  PC_NAME(1),  PC_NAME(2),  PC_NAME(3),  PC_NAME(4),  PC_NAME(5),  PC_NAME(6),  PC_NAME(7),
    PC_NAME(8),  PC_NAME(9),  PC_NAME(10), PC_NAME(11), PC_NAME(12), PC_NAME(13), PC_NAME(14), PC_NAME(15)};


// ------------------------------------------------------------------------------------
// Main program
// ------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
    // ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = (argc != 2) ? "mlp_DDR_pong_1.xclbin" : argv[1];
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &err);
    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    cl::Program program(context, devices, bins, NULL, &err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    cl::Kernel krnl_vector_add(program, "top", &err);

    // ------------------------------------------------------------------------------------
    // Step 2: Create buffers and initialize test values
    // ------------------------------------------------------------------------------------
    //std::vector<blockvec> In_rows(BATCH_SIZE);
    std::vector<float> In_rows(L1_total*BATCH_SIZE);
    //std::vector<blockvec> C_rows(BATCH_SIZE);
    std::vector<float> B1(L2*L1_total);
    std::vector<float> B2(L3*L2);
    std::vector<float> C_rows(L3*BATCH_SIZE);

    printf("here 1\n");
    

    float A_ini[BSIZE*L1_total];
    // float B1_ini[L1*L2];
    // float B2_ini[L2*L3];
//    float C_ini[BSIZE*L4];
 //   float C_ref[BSIZE*L4];
    
    int i, j, k;
    std::cout << "init A_ini matrix." << std::endl;
    for (i = 0; i < BSIZE; i++) {
        for (j = 0; j < L1_total; j++) {
            A_ini[i*L1_total+j] = 1;
        }
    }
    for (i = 0; i < L1_total; i++) {
        for (j = 0; j < BSIZE; j++) {
            In_rows[i*BSIZE+j] = A_ini[i*BSIZE+j];
        }
    }

    std::cout << "init B1_ini matrix." << std::endl;
    for (int it = 0; it < 4; it++){
        for (int i = 0; i < L1_total; i++){
            for (int j = 0; j < L2/4; j++){
                B1[it*L1_total*L2/4+i*L2/4+j]=j/4.0;
            }
        }
    }

    std::cout << "init B2_ini matrix." << std::endl;
    for (int i = 0; i < L2; i++){
        for (int j = 0; j < L3; j++){
            B2[i*L3+j]=j/4.0;
        }
    }
    printf("All inied\n");
    // ===================================Calculating golden result=======================================

    float L1_out[BATCH_SIZE][L2]={0};
    float L2_out[BATCH_SIZE][L3]={0};
    float bias1[L2]={-0.002713746391236782,0.005080224946141243,0.0004187696613371372,0.002765250625088811,-0.009916051290929317,-0.011488431133329868,-0.0013400388415902853,-0.021577484905719757,4.346558853285387e-05,0.005001293495297432,-0.006017204374074936,-0.004392923787236214,-0.006819071713835001,0.00638744980096817,0.00249408814124763,-0.01205851323902607,-0.004362733103334904,-0.018475547432899475,-0.01908177137374878,-0.007495387457311153,-0.006348637863993645,-0.006212342530488968,-0.0019430192187428474,-0.008549263700842857,0.002515411237254739,-0.0031618501525372267,-0.0045793126337230206,0.0073177204467356205,0.0014229478547349572,0.0048087965697050095,0.0015623789513483644,-0.012747623026371002,-0.009565400891005993,-0.00210772268474102,-0.011433745734393597,0.008184936828911304,-0.011166036128997803,-0.00013351303641684353,-0.00283531891182065,-0.005506355315446854,-0.007421841844916344,-0.002563793445006013,-0.025578731670975685,-0.02994558960199356,-0.006186700891703367,-0.0057435305789113045,-0.007784753106534481,0.003479316597804427,-0.008498371578752995,0.016285952180624008,0.012491601519286633,0.027105772867798805,-0.001695004990324378,0.003257863689213991,-0.012198065407574177,-0.005054636392742395,-0.005502903368324041,-0.008389001712203026,-0.0011754181468859315,0.005770880728960037,6.980852776905522e-05,-0.00970427691936493,-0.0026128734461963177,0.0032911242451518774,-0.0008171047666110098,-0.0019441660260781646,0.003921688999980688,0.008867966011166573,-0.009013532660901546,-0.004831379745155573,-0.01548770722001791,-0.004521734081208706,0.003369347658008337,0.00203329767100513,-0.01855839416384697,-0.00038492161547765136,0.0049302838742733,0.0027426721062511206,0.003629702841863036,0.013267380185425282,0.01220780611038208,0.018412014469504356,0.0008072922937572002,-0.01395778451114893,-0.0061768582090735435,-0.0023615718819200993,0.011616542004048824,-0.005581867881119251,-0.009748296812176704,-9.193787263939157e-05,-0.006887709256261587,-0.01023881509900093,-0.012862513773143291,0.004930448718369007,-0.010106847621500492,-0.00348519254475832,0.0015669467393308878,0.011902340687811375,0.0029599976260215044,0.00025102501967921853,-0.012430957518517971,-0.0045038750395178795,0.0002334681776119396,0.0063716997392475605,-0.00926326122134924,-0.0072345309890806675,-0.012615982443094254,-0.011489560827612877,0.0033125276677310467,0.004587702453136444,-0.00236687995493412,-0.0069780778139829636,0.0017995426896959543,-0.007602886762470007,0.002364694606512785,-0.014224877581000328,-0.00439787283539772,0.0016004437347874045,0.001497467397712171,0.003642771393060684,-0.008494473062455654,-0.010078531689941883,-0.010260190814733505,-0.0027102401945739985,-0.0015873651718720794,0.0016823416808620095,-0.0017960106488317251,-0.01867027021944523,-0.00453923037275672,-0.004059880971908569,-0.01189577579498291,0.0052947415970265865,-0.014916712418198586,0.003307005623355508,-0.008669333532452583,-0.0053834435530006886,-0.013174625113606453,0.005942160729318857,-0.0046415687538683414,-0.0017763469368219376,0.0006930945673957467,-0.01184218842536211,-0.013073526322841644,0.00035654802923090756,0.013783627189695835,0.003992204088717699,-0.006444474682211876,-0.014413918368518353,-0.013523286208510399,0.006855919025838375,-0.004742837511003017,-0.008915621787309647,-0.004507176112383604,-0.006434829439967871,-0.012650120072066784,-0.0004001902707386762,0.008143136277794838,0.009340690448880196,0.004880652297288179,-0.010304292663931847,-0.005832995288074017,0.0015519371954724193,-0.0090406509116292,-0.00734327919781208,0.002056376077234745,0.0024600259494036436,0.005347602069377899,-0.000256237864959985,0.00665899645537138,0.0022179163061082363,-0.007676857057958841,-0.018110964447259903,-0.005683806259185076,-0.006123387720435858,0.005140654742717743,-0.00837423000484705,-0.0231422558426857,-0.005560790188610554,-0.012345274910330772,0.010971656069159508,-0.007717514410614967,-0.0011895514326170087,-0.013156807981431484,0.0007771714590489864,-0.009255082346498966,0.0005370446597225964,-0.0015117593575268984,0.001212257775478065,0.0029095052741467953,-0.012441689148545265,-0.025410212576389313,0.0029722615145146847,-0.004348330199718475,0.0037932591512799263,0.002612097654491663,0.00026560985133983195,0.002458008239045739,-0.008994814939796925,0.0031473543494939804,-0.0005555519601330161,0.0016262870049104095,-0.004454184789210558,-0.0024945845361799,-0.012215862981975079,0.005945962853729725,0.004429275635629892,-0.0007699612760916352,-0.00417544599622488,0.0212148055434227,-0.005338952410966158,-0.009549399837851524,0.004751088097691536,0.0024963724426925182,9.22792823985219e-05,0.0009357861708849669,-0.004230358172208071,-0.007060748990625143,0.002019342500716448,0.007335562724620104,0.016144169494509697,-0.0006140721961855888,-0.007368006277829409,-0.006721615791320801,-0.016682691872119904,0.004941405728459358,-0.003667382290586829,0.0009203062509186566,-0.002565731294453144,0.0015367042506113648,-0.00734518188983202,-0.011258527636528015,-0.0040993099100887775,-0.006352203898131847,0.0013305011671036482,-0.0016849038656800985,-0.0005415490595623851,-0.010602817870676517,0.0007205692236311734,0.017739390954375267,-0.009496285580098629,-0.012094645760953426,0.0037481181789189577,-0.007096330635249615,-0.007805733475834131,0.009681282564997673,0.0023009555879980326,-0.005463696550577879,0.003927135374397039,-0.009062036871910095,0.0020932380575686693,-0.0064757694490253925,-0.013251649215817451,-0.008923092857003212,-0.01172816101461649,-0.018200665712356567,0.0005910075269639492,0.002729956526309252,-0.009828944690525532,0.0007334572728723288,-0.005272684618830681,0.0019034093711525202,-0.009643997997045517,0.0006993152783252299,-0.004222061950713396,0.008090300485491753,-0.004020598717033863,0.0037709688767790794,-0.014054608531296253,0.00930807739496231,-0.0005556044634431601,-0.0034010109957307577,-0.0035000103525817394,-0.006122499704360962,0.014579189009964466,0.0006013787351548672,-0.01384483091533184,-0.002896643243730068,-0.00017263612244278193,-0.0017051384784281254,-0.004914804827421904,-0.010413680225610733,-0.0004340493178460747,0.0028567754197865725,-0.011093140579760075,0.012673621065914631,-0.012767140753567219,0.002692590467631817,0.020954132080078125,-0.011112737469375134,-0.004005576483905315,0.018061043694615364,0.0015080058947205544,0.009264349937438965,-8.341179636772722e-05,-0.004826872609555721,-0.013858792372047901,-0.0020321302581578493,-0.008489825762808323,-0.016040310263633728,-0.027370847761631012,-0.0008242321782745421,-0.0010698335245251656,0.0010094555327668786,0.023174051195383072,-0.00761030800640583,-0.016256412491202354,-0.004868939518928528,-0.0025790182407945395,-0.0029468813445419073,-0.01679951883852482,0.021071920171380043,-0.012218340300023556,-0.011118500493466854,-0.0003757263475563377,6.361228588502854e-05,-0.0005979566485621035,-0.00022751049255020916,-0.00037130838609300554,-0.006670496892184019,-0.01253445167094469,-0.013846054673194885,-0.004852450918406248,-0.005305555649101734,-0.005610466469079256,-0.00569887924939394,0.007539889309555292,0.003869602456688881,-0.009564313106238842,-0.011090644635260105,0.0013227775925770402,8.519881521351635e-05,0.0002654888085089624,0.005184595938771963,-0.01293883752077818,-0.012772710993885994,-0.010802377946674824,-0.014802423305809498,-0.007657251786440611,-0.012798696756362915,-0.008196062408387661,-0.004450210835784674,-0.007372723426669836,0.0008577235275879502,-0.002298792125657201,-0.000953841779846698,-0.0018946272321045399,0.013363036327064037,-0.01653256081044674,-0.008914356119930744,-0.003550749970600009,0.0036228455137461424,-0.027089571580290794,-0.00912250205874443,-0.0018309613224118948,-0.0011959286639466882,-0.0030877050012350082,-0.0012742846738547087,0.006088309921324253,0.0006835042731836438,-0.011710245162248611,0.0030054799281060696,-0.012172117829322815,-0.01115301251411438,-0.002680460922420025,0.025446414947509766,-0.015589754097163677,-0.017598187550902367,-0.006947848945856094,0.0009117236477322876,0.0011831065639853477,-0.001697296160273254,-0.0006551197730004787,-0.0009963972261175513,-0.0021384120918810368,0.007906852290034294,-0.008182176388800144,-0.008406797423958778,-0.010525963269174099,0.0006602809298783541,-0.00524611072614789,-0.0031448150984942913,-0.011271439492702484,-0.004165921360254288,-0.0008157072588801384,-0.012333232909440994,-0.01010476890951395,0.003493712982162833,-0.015435216948390007,-0.00313038332387805,-0.010986369103193283,-0.007491046562790871,-0.011539550498127937,-0.014893166720867157,-0.01960090734064579,0.0007258176920004189,0.006656023673713207,-0.0011502125998958945,-0.011305021122097969,-0.00873297918587923,-0.007777730468660593,-0.0001740415027597919,-0.011878843419253826,0.005202293861657381,0.0004587218281812966,-0.013501821085810661,-0.011588663794100285,0.0012447163462638855,-0.01930023916065693,-0.0018862563883885741,-0.00185905781108886,-0.012135524302721024,0.00854549277573824,-0.0005963470903225243,-0.005243820603936911,0.007017169147729874,0.0160627793520689,-0.006062440574169159,-0.004887605085968971,0.008213193155825138,0.0008436907664872706,-0.0032176650129258633,-0.00755520211532712,0.0003476886195130646,0.009846647270023823,-0.001437547616660595,-0.010721147991716862,0.004216266795992851,-0.00012414055527187884,0.0002578997518867254,-0.00817043799906969,-0.013877129182219505,-0.00720814848318696,0.004710044711828232,-0.0035767043009400368,-0.013726460747420788,0.001896757516078651,-0.0010663399007171392,-0.009214978665113449,-0.010121658444404602,-0.025439683347940445,-0.012232397682964802,-0.0007515224860981107,0.01366118062287569,0.0007641977863386273,-0.017699338495731354,-0.005948689766228199,-0.0031652143225073814,-0.006928900722414255,-0.010590810328722,-0.0004730664659291506,0.0057754479348659515,-0.0068548512645065784,-0.013295218348503113,-0.0011989913182333112,-0.0045437877997756,-0.005967109464108944,-0.004460780881345272,-0.004731801338493824,-0.01027575321495533,0.0032469863072037697,-0.004282424226403236,-0.003989067394286394,-0.00924102496355772,-0.00572930509224534,-0.018125521019101143,-0.0034397768322378397,0.001726677524857223,7.398384332191199e-05,-0.011245423927903175,-0.010572497732937336,-0.0035776817239820957,-0.003064611693844199,-0.014875960536301136,-0.014208712615072727,0.0009159045876003802,-0.0037565992679446936,-0.012304951436817646,-0.036240242421627045,-0.005813682917505503,-0.004276437684893608,-0.001130161457695067,-0.012913296930491924,0.007590222172439098,0.005075267981737852,-0.00034683311241678894,-0.016202250495553017,-0.0020443496759980917,-0.0017630856018513441,0.0076440442353487015,-0.0009424259187653661,-0.003915851004421711,0.0009002761216834188,0.001025890582241118,0.008911875076591969,-0.010274655185639858,-0.0006980486796237528,-0.009437421336770058,-0.0002493816427886486,-0.0128041822463274,0.00016224163118749857,-0.010896950028836727,-0.0044518220238387585,0.02322024293243885,0.002736307680606842,-0.010126377455890179,0.020640231668949127,-0.014071434736251831,0.010566274635493755,-0.0005696375737898052,-0.0089614512398839,-0.007708959747105837,-0.00012240279465913773};
    float bias2[L3]={0.018759336322546005,0.0012862527510151267};
    // matmul_layer1
    for (int it = 0; it < 4; it++){
        for (int i = 0; i < BATCH_SIZE; i++){
            for (int j = 0; j < L2/4; j++){
                for (int k = 0; k < L1_total; k++){
                    L1_out[i][it*L2/4+j] += In_rows[k*BSIZE+i] * B1[it*L1_total*L2/4+k*L2/4+j];
                }
                // printf("In,B1: %f, %f\n", In_rows[k*BSIZE+i], B1[it*L1_total*L2/4+k*L2/4+j]);
            }
        }
    }
    // for (int i = 0; i < BATCH_SIZE; i++){
    //     for(int j = 0; j < L2; j++){
    //         printf("%f ", L1_out[i][j]);
    //     }
    // }
    // bias, activation of layer 1
    for (int i = 0; i < BATCH_SIZE; i++){
        for(int j = 0; j < L2; j++){
            // +bias,relu
            L1_out[i][j]=(L1_out[i][j]+bias1[j]<0)? 0 : L1_out[i][j]+bias1[j];
            // printf("%f \n",L1_out[i][j]);
        }
    }
    // matmul layer 2
    for (int i = 0; i < BATCH_SIZE; i++){
        for (int j = 0; j < L3; j++){
            for (int k = 0; k < L2; k++){
                L2_out[i][j] += L1_out[i][k] * B2[k*L3+j];
            }
        }
    }
    //bias of layer 2
    for (int i = 0; i < BATCH_SIZE; i++){
        for(int j = 0; j < L3; j++){
            L2_out[i][j]=L2_out[i][j]+bias2[j];
            printf("%f ",L2_out[i][j]);
        }
    }

    // ================================End calculating golden result=======================================

  
    
    cl_mem_ext_ptr_t InrExt;
    cl_mem_ext_ptr_t B1Ext;
    cl_mem_ext_ptr_t B2Ext;
    cl_mem_ext_ptr_t CrExt;
    

    InrExt.obj = In_rows.data();
    InrExt.param = 0;
    InrExt.flags = 0|XCL_MEM_TOPOLOGY;

    B1Ext.obj = B1.data();
    B1Ext.param = 0;
    B1Ext.flags = 0|XCL_MEM_TOPOLOGY;

    B2Ext.obj = B2.data();
    B2Ext.param = 0;
    B2Ext.flags = 0|XCL_MEM_TOPOLOGY;

    CrExt.obj = C_rows.data();
    CrExt.param = 0;
    CrExt.flags = 1|XCL_MEM_TOPOLOGY;

  printf("flags set\n");
    // Create the buffers and allocate memory
    //cl::Buffer in1_buf(context, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY|CL_MEM_EXT_PTR_XILINX, sizeof(blockvec) * BATCH_SIZE, &InrExt, &err);
    cl::Buffer in1_buf(context, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY|CL_MEM_EXT_PTR_XILINX, sizeof(float) * L1_total*BATCH_SIZE, &InrExt, &err);
    //cl::Buffer out_buf(context, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY|CL_MEM_EXT_PTR_XILINX, sizeof(blockvec) * BATCH_SIZE, &CrExt, &err);
    cl::Buffer b1_buf(context, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY|CL_MEM_EXT_PTR_XILINX, sizeof(float) * L1_total*L2, &B1Ext, &err);
    cl::Buffer b2_buf(context, CL_MEM_USE_HOST_PTR|CL_MEM_READ_ONLY|CL_MEM_EXT_PTR_XILINX, sizeof(float) * L2*L3, &B2Ext, &err);
    cl::Buffer out_buf(context, CL_MEM_USE_HOST_PTR|CL_MEM_WRITE_ONLY|CL_MEM_EXT_PTR_XILINX, sizeof(float) * L3*BATCH_SIZE, &CrExt, &err);
      printf("hi\n");
    // Set kernel arguments
    krnl_vector_add.setArg(0, in1_buf);
    krnl_vector_add.setArg(1, b1_buf);
    krnl_vector_add.setArg(2, b2_buf);
    krnl_vector_add.setArg(3, out_buf);

    // Map host-side buffer memory to user-space pointers [replaced, used equeueMapBuffer]
    //blockvec *A = (blockvec *)q.enqueueMapBuffer(in1_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(blockvec) * BATCH_SIZE);
    //blockvec *B = (blockvec *)q.enqueueMapBuffer(in2_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(blockvec) * BATCH_SIZE);
    //blockmat *C = (blockmat *)q.enqueueMapBuffer(out_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(blockmat) * OUT_SIZE);
    //std::vector<blockvec> A(BATCH_SIZE);
    //std::vector<blockvec> B(BATCH_SIZE);
    //std::vector<blockmat> C(OUT_SIZE);
    
      printf("setArg finished\n");

    // FILE *fp3;
    // fp3=fopen("./IOnkernel.dat","w");

    // for (j = 0; j < L1_total; j++) {
    //     for (i = 0; i < BSIZE; i++) {
    //         fprintf(fp3,"%f ",In_rows[j*BSIZE+i]);
    //     }
    //     fprintf(fp3,"\n");
    // }

    // fclose(fp3);
    // printf("starting kernel\n");
    // ------------------------------------------------------------------------------------
    // Step 3: Run the kernel
    // ------------------------------------------------------------------------------------

    krnl_vector_add.setArg(0, in1_buf);
    krnl_vector_add.setArg(1, b1_buf);
    krnl_vector_add.setArg(2, b2_buf);
    krnl_vector_add.setArg(3, out_buf);
    printf("setArg\n");
    // Schedule transfer of inputs to device memory, execution of kernel, and transfer of outputs back to host memory
    q.enqueueMigrateMemObjects({in1_buf}, 0 /* 0 means from host*/);
    q.enqueueMigrateMemObjects({b1_buf}, 0 /* 0 means from host*/);
    q.enqueueMigrateMemObjects({b2_buf}, 0 /* 0 means from host*/);
    printf("sent data\n");
    q.enqueueTask(krnl_vector_add);
    q.finish();
    printf("executed kernel\n");
    q.enqueueMigrateMemObjects({out_buf}, CL_MIGRATE_MEM_OBJECT_HOST);
    printf("data back\n");

    // Wait for all scheduled operations to finish
    q.finish();
    printf("q.finish\n");
    // ------------------------------------------------------------------------------------
    // Step 4: Check Results and Release Allocated Resources
    // ------------------------------------------------------------------------------------
    bool match = true;
    printf("hi\n");
    FILE *fp;
    fp=fopen("./Crows.dat","w");
    printf("hi\n");
    for (j = 0; j < L3; j++) {
        for (i = 0; i < BSIZE; i++) {
            fprintf(fp, "%f ", C_rows[j*BSIZE+i]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
 
    delete[] fileBuf;

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
    // return (EXIT_SUCCESS);
}

// ------------------------------------------------------------------------------------
// Utility functions
// ------------------------------------------------------------------------------------
std::vector<cl::Device> get_xilinx_devices()
{
    size_t i;
    cl_int err;
    std::vector<cl::Platform> platforms;
    err = cl::Platform::get(&platforms);
    cl::Platform platform;
    for (i = 0; i < platforms.size(); i++)
    {
        platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>(&err);
        if (platformName == "Xilinx")
        {
            std::cout << "INFO: Found Xilinx Platform" << std::endl;
            break;
        }
    }
    if (i == platforms.size())
    {
        std::cout << "ERROR: Failed to find Xilinx platform" << std::endl;
        exit(EXIT_FAILURE);
    }

    //Getting ACCELERATOR Devices and selecting 1st such device
    std::vector<cl::Device> devices;
    err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
    return devices;
}

char *read_binary_file(const std::string &xclbin_file_name, unsigned &nb)
{
    if (access(xclbin_file_name.c_str(), R_OK) != 0)
    {
        printf("ERROR: %s xclbin not available please build\n", xclbin_file_name.c_str());
        exit(EXIT_FAILURE);
    }
    //Loading XCL Bin into char buffer
    std::cout << "INFO: Loading '" << xclbin_file_name << "'\n";
    std::ifstream bin_file(xclbin_file_name.c_str(), std::ifstream::binary);
    bin_file.seekg(0, bin_file.end);
    nb = bin_file.tellg();
    bin_file.seekg(0, bin_file.beg);
    char *buf = new char[nb];
    bin_file.read(buf, nb);
    return buf;
}