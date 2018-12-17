R_GROUP=RIALTO-2
VM_NAME=azure-builder

echo "info - Creating resource group"

az group create --name $R_GROUP --location australiaeast
echo "info - Group created"

az vm create \
    --resource-group $R_GROUP \
    --name $VM_NAME \
    --image Canonical:UbuntuServer:18.04-LTS:18.04.201812060 \
    --generate-ssh-keys \
    --admin-username askap \

IP=`az vm list-ip-addresses --resource-group RIALTO-2 --name azure-builder --output table | tail -1 | awk '{print $2}'`
echo $IP
ssh askap@$IP  screen -dmS remote_session
ssh askap@$IP  screen -r remote_session; git clone https://bitbucket.csiro.au/scm/~ord006/jacal-dev.git ./jacal-dev ; . jacal-dev/Tools/Dev/deploy_azure/build-provision.sh ; askap-build.sh

# insert build instructions here
# probably best to run something via a screen call - but need to report completeness
# other option is to add jenkins to the provisioning and configure it to do the build
# thats probably the best option

#az vm stop --name azure-builder --resource-group RIALTO-2 
#az group delete --name $R_GROUP --no-wait --yes
