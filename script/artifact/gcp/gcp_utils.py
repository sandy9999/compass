import time
import threading
import paramiko
import subprocess

from google.cloud import compute_v1


def create_instance(config, instance_name):

    print(f"Creating instance: {instance_name}.")
    
    instance_client = compute_v1.InstancesClient()
    operation_client = compute_v1.ZoneOperationsClient()

    instance = compute_v1.Instance()
    instance.name = instance_name
    instance.machine_type = f"zones/{config['zone']}/machineTypes/{config['machine_type']}"

    # Configure boot disk
    initialize_params = compute_v1.AttachedDiskInitializeParams(
        source_image=config['boot_disk_image'],
        disk_size_gb=config.get('disk_size_gb', 10)  # Default 10GB
    )
    boot_disk = compute_v1.AttachedDisk(
        boot=True, auto_delete=True, initialize_params=initialize_params
    )
    instance.disks = [boot_disk]

    # Configure network
    network_interface = compute_v1.NetworkInterface()
    network_interface.network = f"projects/{config['project_id']}/global/networks/{config['vpc_name']}"

    # Add access configuration to enable external IP
    access_config = compute_v1.AccessConfig(
        name="External NAT", type_="ONE_TO_ONE_NAT"
    )
    network_interface.access_configs = [access_config]

    
    instance.network_interfaces = [network_interface]

    # Create instance
    operation = instance_client.insert(
        project=config['project_id'], zone=config['zone'], instance_resource=instance
    )

    # Wait for the operation to complete
    operation_client.wait(project=config['project_id'], zone=config['zone'], operation=operation.name)

    # Retrieve the created instance details
    created_instance = instance_client.get(project=config['project_id'], zone=config['zone'], instance=instance_name)

    internal_ip = created_instance.network_interfaces[0].network_i_p
    # instance_internal_ips[instance_name] = internal_ip

    print(f"Instance {instance_name} created successfully with internal IP: {internal_ip}")

    return (instance, internal_ip)

def delete_instance(project_id, zone, instance_name):
    """Deletes a GCP instance."""
    instance_client = compute_v1.InstancesClient()

    try:
        # Make the API request to delete the instance
        operation = instance_client.delete(
            project=project_id,
            zone=zone,
            instance=instance_name,
        )
        print(f"Deleting instance {instance_name}: {operation.operation_type}")

        # Wait for the operation to complete
        operation_client = compute_v1.ZoneOperationsClient()
        operation_client.wait(project=project_id, zone=zone, operation=operation.name)
        print(f"Instance {instance_name} deleted successfully.")
    except Exception as e:
        print(f"Failed to delete instance {instance_name}: {e}")