Building Node Sets from Model Design
------------------------------------
It is recommended to use the precompiled docker container.
Information to use the docker: [build docker](https://github.com/OPCFoundation/UA-ModelCompiler#docker-build)
Copy your model design file inside a new folder. (You can use the example tsn_app_model_design.xml)
Run:
```
sudo docker run --mount type=bind,source=$(pwd),target=/model/src --entrypoint "/app/PublishModel.sh" sailavid/ua-modelcompiler /model/src/tsn_app_model_design TsnApp /model/src/Published
```
This command generates several files in your folder and especially:
 - Published/TsnApp/TsnApp.NodeSet2.xml
 - Published/TsnApp/tsn_app_model_design.csv

Checking Information Model (optional step)
--------------------------
When you are coding the model design file, it's useful to check the Information Model.
Without this step, you can only check the Information Model when you connect the Client to the Server.
1) Make sure python 3.6+ and python-pip are installed.
2) Install the FreeOpcUa Modeler.
``` 
pip3 install opcua-modeler
```
3) Run it:
``` 
opcua-modeler
```
4) Click on Actions->open and open TsnApp.NodeSet2.xml (generated in the previous step).

Generating NodeId Header File
-----------------------------
Clone the git repository for open62541: [open62541](https://github.com/open62541/open62541)
In open62541 folder, run this command with your path to tsn_app_model_design.csv (generated in the first step).
```
tools/generate_nodeid_header.py <your_path>/tsn_app_model_design.csv ./tsn_app_ua_nodeid_header 1
```

Generating C Model (which can be used by open62541)
------------------
In open62541 folder, run the command below with your own path to TsnApp.NodeSet2.xml.
```
tools/nodeset_compiler/nodeset_compiler.py -e tools/schema/Opc.Ua.NodeSet2.Reduced.xml --xml <your_path>/TsnApp.NodeSet2.xml tsn_app_model
```

Finally, three files are generated in src_generated :
	- tsn_app_model.c
	- tsn_app_model.h
	- tsn_app_ua_nodeid_header.h
